#ifndef slic3r_PrinterNetworkManager_hpp_
#define slic3r_PrinterNetworkManager_hpp_

#include "PrinterNetwork.hpp"
#include <map>
#include <memory>
#include <vector>
#include <mutex>

namespace Slic3r {

class PrinterNetworkManager {
public:
    static PrinterNetworkManager* getInstance() {
        static PrinterNetworkManager instance;
        return &instance;
    }

    std::vector<std::map<std::string, std::string>> discoverPrinters();
    bool connectToPrinter(const std::string& printerId, const std::string& printerIp, const std::string& printerPort);
    void disconnectFromPrinter(const std::string& printerId);
    bool sendPrintTask(const std::string& printerId, const std::string& task);
    bool sendPrintFile(const std::string& printerId, const std::string& file);
    bool isPrinterConnected(const std::string& printerId);
    NetworkStatus getPrinterStatus(const std::string& printerId);

private:
    PrinterNetworkManager();
    ~PrinterNetworkManager();
    
    PrinterNetworkManager(const PrinterNetworkManager&) = delete;
    PrinterNetworkManager& operator=(const PrinterNetworkManager&) = delete;

    std::map<std::string, std::unique_ptr<IPrinterNetwork>> m_networkConnections;
    std::mutex m_connectionsMutex;
};

} // namespace Slic3r

#endif
