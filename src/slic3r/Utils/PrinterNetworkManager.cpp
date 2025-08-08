#include "PrinterNetworkManager.hpp"
#include "ElegooNetwork.hpp"
#include "PrinterNetworkEvent.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include <wx/log.h>
#include "PrintHost.hpp"

namespace Slic3r {

PrinterNetworkManager::PrinterNetworkManager()
{
    // connect status changed event
    PrinterNetworkEvent::getInstance()->connectStatusChanged.connect(
        [this](const PrinterConnectStatusEvent& event) { onPrinterConnectStatus(event.printerId, event.status); });

    // printer status changed event
    PrinterNetworkEvent::getInstance()->statusChanged.connect(
        [this](const PrinterStatusEvent& event) { onPrinterStatus(event.printerId, event.status); });

    // printer print task changed event
    PrinterNetworkEvent::getInstance()->printTaskChanged.connect(
        [this](const PrinterPrintTaskEvent& event) { onPrinterPrintTask(event.printerId, event.task); });

    // printer attributes changed event
    PrinterNetworkEvent::getInstance()->attributesChanged.connect(
        [this](const PrinterAttributesEvent& event) { onPrinterAttributes(event.printerId, event.printerInfo); });
}

PrinterNetworkManager::~PrinterNetworkManager() { close(); }

void PrinterNetworkManager::close()
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    for (auto& [printerId, network] : mNetworkConnections) {
        network->disconnectFromPrinter(printerId);
    }
    mNetworkConnections.clear();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> PrinterNetworkManager::discoverPrinters()
{
    std::vector<PrinterNetworkInfo>      printers;
    std::vector<PrinterNetworkErrorCode> errors;
    for (auto& printerHostType : {htElegooLink, htOctoPrint, htPrusaLink, htPrusaConnect, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS,
                                  htESP3D, htCrealityPrint, htObico, htFlashforge, htSimplyPrint}) {
        auto network = PrinterNetworkFactory::createNetwork(printerHostType);
        if (network) {
            auto result = network->discoverDevices();
            if (result.isSuccess() && result.hasData()) {
                printers.insert(printers.end(), result.data.value().begin(), result.data.value().end());
            } else if (result.isError()) {
                errors.push_back(result.code);
                wxLogWarning("Failed to discover devices for host type %d: %s", static_cast<int>(printerHostType), result.message.c_str());
            }
        }
    }
    if (printers.size() > 0) {
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, printers);
    } else if (errors.size() > 0) {
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(errors[0], printers);
    }
    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, printers);
}

PrinterNetworkResult<PrinterNetworkInfo> PrinterNetworkManager::addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto                        it = mNetworkConnections.find(printerNetworkInfo.printerId);
    if (it != mNetworkConnections.end()) {
        wxLogMessage("Printer already added,  printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                     printerNetworkInfo.printerModel);
        it->second->disconnectFromPrinter(printerNetworkInfo.printerId);
        mNetworkConnections.erase(it);
    }
    auto network = PrinterNetworkFactory::createNetwork(PrintHost::get_print_host_type(printerNetworkInfo.hostType));
    if (!network) {
        wxLogError("Failed to create network for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        return PrinterNetworkResult<PrinterNetworkInfo>(PrinterNetworkErrorCode::INTERNAL_ERROR, printerNetworkInfo,
                                          "Failed to create network for host type " + printerNetworkInfo.hostType);
    }
    auto result = network->addPrinter(printerNetworkInfo, connected);
    if (result.isSuccess()) {
        mNetworkConnections[printerNetworkInfo.printerId] = std::move(network);
        wxLogMessage("Added printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName, printerNetworkInfo.printerModel);
    } else {
        wxLogError("Failed to add printer: %s %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel, result.message.c_str());
    }

    return result;
}

PrinterNetworkResult<PrinterNetworkInfo> PrinterNetworkManager::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto                        it = mNetworkConnections.find(printerNetworkInfo.printerId);
    if (it != mNetworkConnections.end()) {
        wxLogMessage("Printer already connected,  printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                     printerNetworkInfo.printerModel);
        it->second->disconnectFromPrinter(printerNetworkInfo.printerId);
        mNetworkConnections.erase(it);
    }
    auto network = PrinterNetworkFactory::createNetwork(PrintHost::get_print_host_type(printerNetworkInfo.hostType));
    if (!network) {
        wxLogError("Failed to create network for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        return PrinterNetworkResult<PrinterNetworkInfo>(PrinterNetworkErrorCode::INTERNAL_ERROR, printerNetworkInfo,
                                          "Failed to create network for host type " + printerNetworkInfo.hostType);
    }
    auto result = network->connectToPrinter(printerNetworkInfo);
    if (result.isSuccess()) {
        mNetworkConnections[printerNetworkInfo.printerId] = std::move(network);
        wxLogMessage("Connected to printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName, printerNetworkInfo.printerModel);
    } else {
        wxLogError("Failed to connect to printer: %s %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel, result.message.c_str());
    }

    return result;
}

PrinterNetworkResult<bool> PrinterNetworkManager::disconnectFromPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    PrinterNetworkResult<bool>  result(PrinterNetworkErrorCode::UNKNOWN_ERROR, false);
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto                        it = mNetworkConnections.find(printerNetworkInfo.printerId);
    if (it != mNetworkConnections.end()) {
        result = it->second->disconnectFromPrinter(printerNetworkInfo.printerId);
        mNetworkConnections.erase(it);
        if (result.isSuccess()) {
            wxLogMessage("Disconnected from printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel);
        } else {
            wxLogWarning("Disconnected from printer %s %s %s but encountered error: %s", printerNetworkInfo.host,
                         printerNetworkInfo.printerName, printerNetworkInfo.printerModel, result.message.c_str());
        }
    } else {
        wxLogError("No network connection for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
    }
    return result;
}

PrinterNetworkResult<bool> PrinterNetworkManager::sendPrintTask(const PrinterNetworkInfo&   printerNetworkInfo,
                                                                const PrinterNetworkParams& params)
{
    std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(printerNetworkInfo.printerId);
    PrinterNetworkResult<bool>       result(PrinterNetworkErrorCode::UNKNOWN_ERROR, false);
    if (network) {
        result = network->sendPrintTask(printerNetworkInfo, params);
        if (result.isError()) {
            wxLogError("Failed to send print task to printer %s %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                       printerNetworkInfo.printerModel, result.message.c_str());
        }
    } else {
        wxLogError("No network connection for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
    }
    return result;
}

PrinterNetworkResult<bool> PrinterNetworkManager::sendPrintFile(const PrinterNetworkInfo&   printerNetworkInfo,
                                                                const PrinterNetworkParams& params)
{
    std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(printerNetworkInfo.printerId);
    PrinterNetworkResult<bool>       result(PrinterNetworkErrorCode::UNKNOWN_ERROR, false);
    if (network) {
        result = network->sendPrintFile(printerNetworkInfo, params);
        if (result.isError()) {
            wxLogError("Failed to send print file to printer %s %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                       printerNetworkInfo.printerModel, result.message.c_str());
        }
    } else {
        wxLogError("No network connection for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
    }
    return result;
}

std::shared_ptr<IPrinterNetwork> PrinterNetworkManager::getPrinterNetwork(const std::string& printerId)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto                        it = mNetworkConnections.find(printerId);
    if (it != mNetworkConnections.end()) {
        return it->second;
    }
    return nullptr;
}
int PrinterNetworkManager::getDeviceType(const PrinterNetworkInfo& printerNetworkInfo)
{
    auto network = PrinterNetworkFactory::createNetwork(PrintHost::get_print_host_type(printerNetworkInfo.hostType));
    if (network) {
        return network->getDeviceType(printerNetworkInfo);
    }
    return -1;
}

void PrinterNetworkManager::registerCallBack(const PrinterConnectStatusFn& printerConnectStatusCallback,
                                             const PrinterStatusFn&        printerStatusCallback,
                                             const PrinterPrintTaskFn&     printerPrintTaskCallback,
                                             const PrinterAttributesFn&     printerAttributesCallback)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    if (!printerConnectStatusCallback || !printerStatusCallback || !printerPrintTaskCallback || !printerAttributesCallback) {
        wxLogError("Invalid callback functions provided to PrinterNetworkManager::registerCallBack");
        return;
    }

    mPrinterConnectStatusCallback = printerConnectStatusCallback;
    mPrinterStatusCallback        = printerStatusCallback;
    mPrinterPrintTaskCallback     = printerPrintTaskCallback;
    mPrinterAttributesCallback   = printerAttributesCallback;

    wxLogMessage("PrinterNetworkManager callbacks registered successfully");
}

void PrinterNetworkManager::onPrinterConnectStatus(const std::string& printerId, const PrinterConnectStatus& status)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    if (mPrinterConnectStatusCallback) {
        mPrinterConnectStatusCallback(printerId, status);
    }
}

void PrinterNetworkManager::onPrinterStatus(const std::string& printerId, const PrinterStatus& status)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    if (mPrinterStatusCallback) {
        mPrinterStatusCallback(printerId, status);
    }
}

void PrinterNetworkManager::onPrinterPrintTask(const std::string& printerId, const PrinterPrintTask& task)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    if (mPrinterPrintTaskCallback) {
        mPrinterPrintTaskCallback(printerId, task);
    }
}

void PrinterNetworkManager::onPrinterAttributes(const std::string& printerId, const PrinterNetworkInfo& printerInfo)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    if (mPrinterAttributesCallback) {
        mPrinterAttributesCallback(printerId, printerInfo);
    }
}

} // namespace Slic3r