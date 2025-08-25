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
    void init();
    void uninit();

    PrinterNetworkResult<PrinterNetworkInfo>              connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool>                            disconnectFromPrinter(const std::string& printerId);
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters();
    PrinterNetworkResult<bool>                            sendPrintTask(const PrinterNetworkParams& params);
    PrinterNetworkResult<bool>                            sendPrintFile(const PrinterNetworkParams& params);
    PrinterNetworkResult<PrinterMmsGroup>                 getPrinterMmsInfo(const std::string& printerId);
    PrinterNetworkResult<PrinterAttributes>               getPrinterAttributes(const std::string& printerId);
    int getPrinterType(const PrinterNetworkInfo& printerNetworkInfo);

private:
    bool isBusy(const std::string& printerId, PrinterStatus& status, int tryCount = 10);
};

} // namespace Slic3r

#endif // slic3r_ElegooLink_hpp_