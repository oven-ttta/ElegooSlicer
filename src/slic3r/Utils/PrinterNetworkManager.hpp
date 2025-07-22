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

    std::vector<PrinterInfo> discoverPrinters();
    bool connectToPrinter(const PrinterInfo& printerInfo);
    bool isPrinterConnected(const PrinterInfo& printerInfo);
    void disconnectFromPrinter(const PrinterInfo& printerInfo);
    
    bool sendPrintTask(const PrinterInfo& printerInfo, const PrinterNetworkParams& params);
    bool sendPrintFile(const PrinterInfo& printerInfo, const PrinterNetworkParams& params);
 

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
