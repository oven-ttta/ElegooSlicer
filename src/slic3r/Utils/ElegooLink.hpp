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
public:
    ElegooLink();
    ~ElegooLink();

    PrinterNetworkResult<PrinterNetworkInfo> addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected);
    PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool> disconnectFromPrinter(const std::string& printerId);
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverDevices();
    PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
    PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
    PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const PrinterNetworkInfo& printerNetworkInfo);
    void close();

    static int getDeviceType(const PrinterNetworkInfo& printerNetworkInfo);
    
    private:
       bool isBusy(const std::string& printerId, PrinterStatus &status, int tryCount = 10);
       std::mutex mMutex;
       bool mIsCleanup = false;
};

} // namespace Slic3r

#endif // slic3r_ElegooLink_hpp_