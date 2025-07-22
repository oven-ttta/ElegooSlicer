#include "PrinterNetworkManager.hpp"
#include "ElegooNetwork.hpp"
#include <wx/log.h>

namespace Slic3r {


PrintHostType getPrinterHostType(const PrinterInfo& printerInfo) {
    if (printerInfo.vendor == "Elegoo") {
        return htElegooLink;
    }
    return htElegooLink;
}

PrinterNetworkManager::PrinterNetworkManager() {
    
}

PrinterNetworkManager::~PrinterNetworkManager() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_networkConnections.clear();
}

std::vector<PrinterInfo> PrinterNetworkManager::discoverPrinters() {
    std::vector<PrinterInfo> printers;
    for (auto& printerHostType : {htElegooLink, htOctoPrint, htPrusaLink, htPrusaConnect, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS, htESP3D, htCrealityPrint, htObico, htFlashforge, htSimplyPrint}) {
        PrinterInfo printerInfo;
        auto network = PrinterNetworkFactory::createNetwork(printerInfo, printerHostType);
        if (network) {
            std::vector<PrinterInfo> printerList = network->discoverDevices();
            printers.insert(printers.end(), printerList.begin(), printerList.end());
        }
    }
    return printers;
}

bool PrinterNetworkManager::connectToPrinter(const PrinterInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerInfo.id);
    if (it != m_networkConnections.end()) {
        wxLogMessage("Printer %s already connected", printerInfo.id);
        return true;
    }
    
    auto network = PrinterNetworkFactory::createNetwork(printerInfo, getPrinterHostType(printerInfo));
    if (!network) {
        wxLogError("Failed to create network for printer: %s", printerInfo.id);
        return false;
    }
    
    if (network->connect()) {
        m_networkConnections[printerInfo.id] = std::move(network);
        wxLogMessage("Connected to printer: %s (%s:%s)", printerInfo.id, printerInfo.ip, printerInfo.port);
        return true;
    } else {
        wxLogError("Failed to connect to printer: %s (%s:%s)", printerInfo.id, printerInfo.ip, printerInfo.port);
        return false;
    }
}

void PrinterNetworkManager::disconnectFromPrinter(const PrinterInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerInfo.id);
    if (it != m_networkConnections.end()) {
        it->second->disconnect();
        m_networkConnections.erase(it);
        wxLogMessage("Disconnected from printer: %s", printerInfo.id);
    }
}

bool PrinterNetworkManager::sendPrintTask(const PrinterInfo& printerInfo, const PrinterNetworkParams& params) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerInfo.id);
    if (it != m_networkConnections.end()) {
        return it->second->sendPrintTask(params);
    } else {
        wxLogError("No network connection for printer: %s", printerInfo.id);
        return false;
    }
}

bool PrinterNetworkManager::sendPrintFile(const PrinterInfo& printerInfo, const PrinterNetworkParams& params) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerInfo.id);
    if (it != m_networkConnections.end()) {
        return it->second->sendPrintFile(params);
    } else {
        wxLogError("No network connection for printer: %s", printerInfo.id);
        return false;
    }
}


bool PrinterNetworkManager::isPrinterConnected(const PrinterInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
 
    auto it = m_networkConnections.find(printerInfo.id);
    if (it != m_networkConnections.end()) {
        return it->second->isConnected();
    }
    return false;
}


} // namespace Slic3r