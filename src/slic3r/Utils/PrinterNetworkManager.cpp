#include "PrinterNetworkManager.hpp"
#include "ElegooNetwork.hpp"
#include <wx/log.h>

namespace Slic3r {


PrintHostType getPrinterHostType(const PrinterNetworkInfo& printerNetworkInfo) {
    if (printerNetworkInfo.vendor == "Elegoo") {
        return htElegooLink;
    }
    return htOctoPrint;
}

PrinterNetworkManager::PrinterNetworkManager() {
    
}

PrinterNetworkManager::~PrinterNetworkManager() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_networkConnections.clear();
}

std::vector<PrinterNetworkInfo> PrinterNetworkManager::discoverPrinters() {
    std::vector<PrinterNetworkInfo> printers;
    for (auto& printerHostType : {htElegooLink, htOctoPrint, htPrusaLink, htPrusaConnect, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS, htESP3D, htCrealityPrint, htObico, htFlashforge, htSimplyPrint}) {
        PrinterNetworkInfo printerNetworkInfo;
        auto network = PrinterNetworkFactory::createNetwork(printerNetworkInfo, printerHostType);
        if (network) {
            std::vector<PrinterNetworkInfo> printerList = network->discoverDevices();
            printers.insert(printers.end(), printerList.begin(), printerList.end());
        }
    }
    return printers;
}

bool PrinterNetworkManager::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerNetworkInfo.id);
    if (it != m_networkConnections.end()) {
        wxLogMessage("Printer %s already connected", printerNetworkInfo.id);
        return true;
    }
    
    auto network = PrinterNetworkFactory::createNetwork(printerNetworkInfo, getPrinterHostType(printerNetworkInfo));
    if (!network) {
        wxLogError("Failed to create network for printer: %s", printerNetworkInfo.id);
        return false;
    }
    
    if (network->connect()) {
        m_networkConnections[printerNetworkInfo.id] = std::move(network);
        wxLogMessage("Connected to printer: %s (%s:%s)", printerNetworkInfo.id, printerNetworkInfo.ip, printerNetworkInfo.port);
        return true;
    } else {
        wxLogError("Failed to connect to printer: %s (%s:%s)", printerNetworkInfo.id, printerNetworkInfo.ip, printerNetworkInfo.port);
        return false;
    }
}

void PrinterNetworkManager::disconnectFromPrinter(const PrinterNetworkInfo& printerNetworkInfo) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerNetworkInfo.id);
    if (it != m_networkConnections.end()) {
        it->second->disconnect();
        m_networkConnections.erase(it);
        wxLogMessage("Disconnected from printer: %s", printerNetworkInfo.id);
    }
}

bool PrinterNetworkManager::sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerNetworkInfo.id);
    if (it != m_networkConnections.end()) {
        return it->second->sendPrintTask(params);
    } else {
        wxLogError("No network connection for printer: %s", printerNetworkInfo.id);
        return false;
    }
}

bool PrinterNetworkManager::sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerNetworkInfo.id);
    if (it != m_networkConnections.end()) {
        return it->second->sendPrintFile(params);
    } else {
        wxLogError("No network connection for printer: %s", printerNetworkInfo.id);
        return false;
    }
}


bool PrinterNetworkManager::isPrinterConnected(const PrinterNetworkInfo& printerNetworkInfo) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
 
    auto it = m_networkConnections.find(printerNetworkInfo.id);
    if (it != m_networkConnections.end()) {
        return it->second->isConnected();
    }
    return false;
}


} // namespace Slic3r