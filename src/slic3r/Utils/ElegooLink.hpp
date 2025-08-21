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
    PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool> disconnectFromPrinter(const std::string& printerId);
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters();
    PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkParams& params);
    PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkParams& params);
    PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const std::string& printerId);
    void close();

    static int getPrinterType(const PrinterNetworkInfo& printerNetworkInfo);
    
    private:
       bool isBusy(const std::string& printerId, PrinterStatus &status, int tryCount = 10);
       std::mutex mMutex;
       bool mIsCleanup = false;
};

} // namespace Slic3r

#endif // slic3r_ElegooLink_hpp_