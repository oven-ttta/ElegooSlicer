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
    
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters();
    PrinterNetworkResult<bool> addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool &connected);
    PrinterNetworkResult<bool> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool> disconnectFromPrinter(const std::string& printerId);
    PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
    PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
    void registerCallBack(const PrinterConnectStatusFn& printerConnectStatusCallback, const PrinterStatusFn& printerStatusCallback, const PrinterPrintTaskFn& printerPrintTaskCallback);
    void close();

private:

    std::mutex mCallbackMutex;
    std::mutex mConnectionsMutex;
    std::map<std::string, std::shared_ptr<IPrinterNetwork>> mNetworkConnections;
    PrinterConnectStatusFn mPrinterConnectStatusCallback;
    PrinterStatusFn mPrinterStatusCallback;
    PrinterPrintTaskFn mPrinterPrintTaskCallback;

    PrinterNetworkManager();
    PrinterNetworkManager(const PrinterNetworkManager&) = delete;
    PrinterNetworkManager& operator=(const PrinterNetworkManager&) = delete;
    std::shared_ptr<IPrinterNetwork> getPrinterNetwork(const std::string& printerId);
    void onPrinterConnectStatus(const std::string& printerId, const PrinterConnectStatus& status);
    void onPrinterStatus(const std::string& printerId, const PrinterStatus& status);
    void onPrinterPrintTask(const std::string& printerId, const PrinterPrintTask& task);
};

} // namespace Slic3r

#endif
