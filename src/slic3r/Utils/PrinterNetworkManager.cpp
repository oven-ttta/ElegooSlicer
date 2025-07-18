#include "PrinterNetworkManager.hpp"
#include "ElegooNetwork.hpp"
#include <wx/log.h>

namespace Slic3r {

PrinterNetworkManager::PrinterNetworkManager() {
    
}

PrinterNetworkManager::~PrinterNetworkManager() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_networkConnections.clear();
}

std::vector<std::map<std::string, std::string>> PrinterNetworkManager::discoverPrinters() {
    std::vector<std::map<std::string, std::string>> printers;
    for (auto& printerHostType : {htElegooLink, htOctoPrint, htPrusaLink, htPrusaConnect, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS, htESP3D, htCrealityPrint, htObico, htFlashforge, htSimplyPrint}) {
        auto network = PrinterNetworkFactory::createNetwork(printerHostType);
        if (network) {
            std::vector<std::map<std::string, std::string>> printerList = network->discoverDevices();
            printers.insert(printers.end(), printerList.begin(), printerList.end());
        }
    }
    return printers;
}

bool PrinterNetworkManager::connectToPrinter(const std::string& printerId, const std::string& printerIp, const std::string& printerPort) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerId);
    if (it != m_networkConnections.end()) {
        wxLogMessage("Printer %s already connected", printerId);
        return true;
    }
    
    auto network = PrinterNetworkFactory::createNetwork(htElegooLink);
    if (!network) {
        wxLogError("Failed to create network for printer: %s", printerId);
        return false;
    }
    
    if (network->connect(printerId, printerIp, printerPort)) {
        m_networkConnections[printerId] = std::move(network);
        wxLogMessage("Connected to printer: %s (%s:%s)", printerId, printerIp, printerPort);
        return true;
    } else {
        wxLogError("Failed to connect to printer: %s (%s:%s)", printerId, printerIp, printerPort);
        return false;
    }
}

void PrinterNetworkManager::disconnectFromPrinter(const std::string& printerId) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerId);
    if (it != m_networkConnections.end()) {
        it->second->disconnect();
        m_networkConnections.erase(it);
        wxLogMessage("Disconnected from printer: %s", printerId);
    }
}

bool PrinterNetworkManager::sendPrintTask(const std::string& printerId, const std::string& task) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerId);
    if (it != m_networkConnections.end()) {
        return it->second->sendPrintTask(task);
    } else {
        wxLogError("No network connection for printer: %s", printerId);
        return false;
    }
}

bool PrinterNetworkManager::sendPrintFile(const std::string& printerId, const std::string& file) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerId);
    if (it != m_networkConnections.end()) {
        return it->second->sendPrintFile(file);
    } else {
        wxLogError("No network connection for printer: %s", printerId);
        return false;
    }
}

bool PrinterNetworkManager::isPrinterConnected(const std::string& printerId) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerId);
    if (it != m_networkConnections.end()) {
        return it->second->isConnected();
    }
    return false;
}

NetworkStatus PrinterNetworkManager::getPrinterStatus(const std::string& printerId) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_networkConnections.find(printerId);
    if (it != m_networkConnections.end()) {
        return it->second->getStatus();
    }
    return NetworkStatus::DISCONNECTED;
}

} // namespace Slic3r