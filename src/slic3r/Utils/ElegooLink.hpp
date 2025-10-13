#ifndef slic3r_ElegooLink_hpp_
#define slic3r_ElegooLink_hpp_

#include "Singleton.hpp"
#include <string>
#include <vector>
#include "libslic3r/PrinterNetworkInfo.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"

namespace Slic3r {

class ElegooLink : public Singleton<ElegooLink>
{
    friend class Singleton<ElegooLink>;
public:
    ElegooLink();
    ~ElegooLink();
    void init();
    void uninit();

    PrinterNetworkResult<PrinterNetworkInfo>              connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool>                            disconnectFromPrinter(const std::string& printerId, bool isWan = false);
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters();
    PrinterNetworkResult<bool>                            sendPrintTask(const PrinterNetworkParams& params, bool isWan = false);
    PrinterNetworkResult<bool>                            sendPrintFile(const PrinterNetworkParams& params, bool isWan = false);
    PrinterNetworkResult<PrinterMmsGroup>                 getPrinterMmsInfo(const std::string& printerId, bool isWan = false);
    PrinterNetworkResult<PrinterNetworkInfo>              getPrinterAttributes(const std::string& printerId, bool isWan = false);
    PrinterNetworkResult<PrinterPrintFileResponse> getFileList(const std::string& printerId, int pageNumber, int pageSize);
    PrinterNetworkResult<PrinterPrintTaskResponse> getPrintTaskList(const std::string& printerId, int pageNumber, int pageSize);
    PrinterNetworkResult<bool> deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds);

    PrinterNetworkResult<std::string> hasInstalledPlugin();
    PrinterNetworkResult<bool> installPlugin(const std::string& pluginPath);
    PrinterNetworkResult<bool> uninstallPlugin();
    PrinterNetworkResult<UserNetworkInfo> loginWAN(const UserNetworkInfo& userInfo);
    PrinterNetworkResult<UserNetworkInfo> getRtcToken();
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> getPrinters();
    PrinterNetworkResult<bool> sendRtmMessage(const std::string& printerId, const std::string& message);
    PrinterNetworkResult<PrinterNetworkInfo> bindWANPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool> unbindWANPrinter(const std::string& printerId);

private:
    bool isBusy(const std::string& printerId, PrinterStatus& status, int tryCount = 10, bool isWan = false);

    std::mutex mMutex;
    bool mIsInitialized;
};

} // namespace Slic3r

#endif // slic3r_ElegooLink_hpp_