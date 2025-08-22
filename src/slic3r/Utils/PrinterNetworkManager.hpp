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
    PrinterNetworkManager(const PrinterNetworkManager&) = delete;
    PrinterNetworkManager& operator=(const PrinterNetworkManager&) = delete;
    PrinterNetworkManager(PrinterNetworkManager&&) = delete;
    PrinterNetworkManager& operator=(PrinterNetworkManager&&) = delete;
    ~PrinterNetworkManager();  

    void init();
    void close();

    PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo, std::shared_ptr<IPrinterNetwork> &network);
    bool addPrinter(const PrinterNetworkInfo& printerNetworkInfo, const std::shared_ptr<IPrinterNetwork> &network);

    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters();
    PrinterNetworkResult<bool> deletePrinter(const std::string& printerId);
    PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkParams& params);
    PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkParams& params);
    PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const std::string& printerId);
    PrinterNetworkResult<PrinterAttributes> getPrinterAttributes(const std::string& printerId);
    
    static int getPrinterType(const PrinterNetworkInfo& printerNetworkInfo);

private:

    PrinterNetworkManager();
  
    bool addPrinterNetwork(const std::shared_ptr<IPrinterNetwork>& network);
    bool deletePrinterNetwork(const std::string& printerId);
    std::shared_ptr<IPrinterNetwork> getPrinterNetwork(const std::string& printerId);

    std::mutex mConnectionsMutex;
    std::map<std::string, std::shared_ptr<IPrinterNetwork>> mNetworkConnections;

    // thread to monitor printer connections
    std::atomic<bool> mIsRunning;
    std::thread mConnectionThread;
    void monitorPrinterConnections();

};

} // namespace Slic3r

#endif
