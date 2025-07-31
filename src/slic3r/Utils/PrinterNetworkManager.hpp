#ifndef slic3r_PrinterNetworkManager_hpp_
#define slic3r_PrinterNetworkManager_hpp_

#include "PrinterNetwork.hpp"
#include <map>
#include <memory>
#include <vector>
#include <mutex>
#include "Singleton.hpp"
namespace Slic3r {

class PrinterNetworkManager : public Singleton<PrinterNetworkManager> {
    friend class Singleton<PrinterNetworkManager>;
public:
    ~PrinterNetworkManager();

    std::vector<PrinterNetworkInfo> discoverPrinters();
    bool connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    bool isPrinterConnected(const PrinterNetworkInfo& printerNetworkInfo);
    void disconnectFromPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    
    bool sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
    bool sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
 

private:
    PrinterNetworkManager();

    std::map<std::string, std::unique_ptr<IPrinterNetwork>> m_networkConnections;
    std::mutex m_connectionsMutex;
};

} // namespace Slic3r

#endif
