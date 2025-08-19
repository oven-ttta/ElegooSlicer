#include "PrinterNetworkManager.hpp"
#include "ElegooNetwork.hpp"
#include "PrinterNetworkEvent.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include <wx/log.h>
#include "PrintHost.hpp"
#include "PrinterCache.hpp"
namespace Slic3r {

PrinterNetworkManager::PrinterNetworkManager()
{
    
}

PrinterNetworkManager::~PrinterNetworkManager() { close(); }

void PrinterNetworkManager::init()
{
    // connect status changed event
    PrinterNetworkEvent::getInstance()->connectStatusChanged.connect(
        [this](const PrinterConnectStatusEvent& event) { PrinterCache::getInstance()->updatePrinterConnectStatus(event.printerId, event.status); });

    // printer status changed event
    PrinterNetworkEvent::getInstance()->statusChanged.connect(
        [this](const PrinterStatusEvent& event) { PrinterCache::getInstance()->updatePrinterStatus(event.printerId, event.status); });

    // printer print task changed event
    PrinterNetworkEvent::getInstance()->printTaskChanged.connect(
        [this](const PrinterPrintTaskEvent& event) { PrinterCache::getInstance()->updatePrinterPrintTask(event.printerId, event.task); });

    // printer attributes changed event
    PrinterNetworkEvent::getInstance()->attributesChanged.connect(
        [this](const PrinterAttributesEvent& event) { PrinterCache::getInstance()->updatePrinterAttributes(event.printerId, event.printerInfo); });  

    mIsRunning = true;
    mConnectionThread = std::thread([this]() { monitorPrinterConnections(); });
}

void PrinterNetworkManager::close()
{  
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    if(!mIsRunning) {
        return;
    }
    mIsRunning = false;
    if (mConnectionThread.joinable()) {
        mConnectionThread.join();
    }
    for (const auto& [printerId, network] : mNetworkConnections) {
        network->disconnectFromPrinter();
    }
    for (const auto& [printerId, network] : mNetworkConnections) {
        network->close();
    }
    mNetworkConnections.clear();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> PrinterNetworkManager::discoverPrinters()
{
    std::vector<PrinterNetworkInfo>      printers;
    std::vector<PrinterNetworkErrorCode> errors;
    for (const auto& printerHostType : {htElegooLink, htOctoPrint, htPrusaLink, htPrusaConnect, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS,
                                  htESP3D, htCrealityPrint, htObico, htFlashforge, htSimplyPrint}) {
        PrinterNetworkInfo printerNetworkInfo;
        printerNetworkInfo.hostType = PrintHost::get_print_host_type_str(printerHostType);
        std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
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

bool PrinterNetworkManager::addPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
    if (!network) {
        wxLogError("Failed to create network for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        return false;
    }

    addPrinterNetwork(network);
    wxLogMessage("Added printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName, printerNetworkInfo.printerModel);
    return true;
}
PrinterNetworkResult<PrinterNetworkInfo> PrinterNetworkManager::connectToPrinter(const PrinterNetworkInfo&  printerNetworkInfo)
{
    std::shared_ptr<IPrinterNetwork> network;
    return connectToPrinter(printerNetworkInfo, network);
}
PrinterNetworkResult<bool> PrinterNetworkManager::deletePrinter(const std::string& printerId)
{
    PrinterNetworkResult<bool> result(PrinterNetworkErrorCode::UNKNOWN_ERROR, false);
    std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(printerId);
    if (!network) {
        wxLogError("No network connection for printer: %s", printerId.c_str());
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
    }
    result = network->disconnectFromPrinter();
    deletePrinterNetwork(printerId);
    if (result.isSuccess()) {
        wxLogMessage("Disconnected from printer: %s %s %s", network->getPrinterNetworkInfo().host,
                     network->getPrinterNetworkInfo().printerName, network->getPrinterNetworkInfo().printerModel);
    } else {
        wxLogWarning("Disconnected from printer %s %s %s but encountered error: %s", network->getPrinterNetworkInfo().host,
                     network->getPrinterNetworkInfo().printerName, network->getPrinterNetworkInfo().printerModel, result.message.c_str());
    }

    return result;
}

PrinterNetworkResult<bool> PrinterNetworkManager::sendPrintTask(const PrinterNetworkParams& params)
{
    std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(params.printerId);
    PrinterNetworkResult<bool>       result(PrinterNetworkErrorCode::UNKNOWN_ERROR, false);
    if (network) {
        result = network->sendPrintTask(params);
        if (result.isError()) {
            wxLogError("Failed to send print task to printer %s %s %s %s", network->getPrinterNetworkInfo().host, network->getPrinterNetworkInfo().printerName,
                       network->getPrinterNetworkInfo().printerModel, result.message.c_str());
        }
    } else {
        wxLogError("No network connection for printer: %s", params.printerId.c_str());
        result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
    }
    return result;
}

PrinterNetworkResult<bool> PrinterNetworkManager::sendPrintFile(const PrinterNetworkParams& params)
{
    std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(params.printerId);
    PrinterNetworkResult<bool>       result(PrinterNetworkErrorCode::UNKNOWN_ERROR, false);
    if (network) {
        result = network->sendPrintFile(params);
        if (result.isError()) {
            wxLogError("Failed to send print file to printer %s %s %s %s", network->getPrinterNetworkInfo().host, network->getPrinterNetworkInfo().printerName,
                       network->getPrinterNetworkInfo().printerModel, result.message.c_str());
        }
    } else {
        wxLogError("No network connection for printer: %s", params.printerId.c_str());
        result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
    }
    return result;
}


PrinterNetworkResult<PrinterMmsGroup> PrinterNetworkManager::getPrinterMmsInfo(const std::string& printerId)
{
    std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(printerId);
    PrinterNetworkResult<PrinterMmsGroup> result(PrinterNetworkErrorCode::UNKNOWN_ERROR, PrinterMmsGroup());
    if (network) {
        result = network->getPrinterMmsInfo();
        if (result.isError()) {
            wxLogError("Failed to get printer mms info for printer %s %s %s %s", network->getPrinterNetworkInfo().host, network->getPrinterNetworkInfo().printerName,
                       network->getPrinterNetworkInfo().printerModel, result.message.c_str());
        }
    } else {
        wxLogError("No network connection for printer: %s", printerId.c_str());
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::NOT_FOUND, PrinterMmsGroup());
    }
    return result;
}

void PrinterNetworkManager::monitorPrinterConnections()
{
    while (mIsRunning) {
        auto printerList = PrinterCache::getInstance()->getPrinters();
        std::vector<std::future<void>> connectionFutures;
        for (const auto& printer : printerList) {
            std::shared_ptr<IPrinterNetwork> activeNetwork = getPrinterNetwork(printer.printerId);
            if (printer.connectStatus == PRINTER_CONNECT_STATUS_CONNECTED && activeNetwork && activeNetwork->getPrinterNetworkInfo().host == printer.host) {
                // already connected and the host is the same, no need to connect again
                continue;
            }
            auto future = std::async(std::launch::async, [this, printer]() {
                // if printer is connected, disconnect it first
                if(printer.connectStatus == PRINTER_CONNECT_STATUS_CONNECTED) {
                    PrinterCache::getInstance()->updatePrinterConnectStatus(printer.printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
                    deletePrinter(printer.printerId);
                }
                std::shared_ptr<IPrinterNetwork> network;
                auto result = connectToPrinter(printer, network);
                if (result.isSuccess()) {
                    addPrinterNetwork(network);
                    PrinterCache::getInstance()->updatePrinterConnectStatus(printer.printerId, PRINTER_CONNECT_STATUS_CONNECTED);
                } 
            });
            connectionFutures.push_back(std::move(future));
        }
        for (auto& future : connectionFutures) {
            future.wait();
        }         
        {
            // disconnect from printers that are not in the printer list or the host is different, mutex lock is needed
            std::lock_guard<std::mutex> lock(mConnectionsMutex);
            for (const auto& [printerId, network] : mNetworkConnections) {
               auto printer = PrinterCache::getInstance()->getPrinter(printerId);
               if (!printer.has_value() || printer.value().host != network->getPrinterNetworkInfo().host) {
                    network->disconnectFromPrinter();
                    deletePrinterNetwork(printerId);
                    wxLogWarning("Host changed or printer not in list, disconnect from printer: %s %s %s", network->getPrinterNetworkInfo().host,
                                 network->getPrinterNetworkInfo().printerName, network->getPrinterNetworkInfo().printerModel);
               } 
            }
        }          
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

PrinterNetworkResult<PrinterNetworkInfo> PrinterNetworkManager::connectToPrinter(const PrinterNetworkInfo&  printerNetworkInfo, std::shared_ptr<IPrinterNetwork>& network)
{
    network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
    if (!network) {
        wxLogError("Failed to create network for printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel);
        return PrinterNetworkResult<PrinterNetworkInfo>(PrinterNetworkErrorCode::INTERNAL_ERROR, printerNetworkInfo,
                                                        "Failed to create network for host type " + printerNetworkInfo.hostType);
    }
    auto result = network->connectToPrinter();
    if (result.isSuccess()) {
        wxLogMessage("Connected to printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                     printerNetworkInfo.printerModel);
    } else {
        wxLogError("Failed to connect to printer: %s %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel, result.message.c_str());
    }
    return result;
}

bool PrinterNetworkManager::addPrinterNetwork(const std::shared_ptr<IPrinterNetwork>& network)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    mNetworkConnections[network->getPrinterNetworkInfo().printerId] = network;
    return true;
}
bool PrinterNetworkManager::deletePrinterNetwork(const std::string& printerId)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    mNetworkConnections.erase(printerId);
    return true;
}

std::shared_ptr<IPrinterNetwork> PrinterNetworkManager::getPrinterNetwork(const std::string& printerId)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto it = mNetworkConnections.find(printerId);

    if (it != mNetworkConnections.end()) {
        return it->second;
    }
    return nullptr;
}

int PrinterNetworkManager::getDeviceType(const PrinterNetworkInfo& printerNetworkInfo)
{
    auto network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
    if (network) {
        return network->getDeviceType();
    }
    return -1;
}


} // namespace Slic3r