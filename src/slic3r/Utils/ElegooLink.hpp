#ifndef slic3r_ElegooLink_hpp_
#define slic3r_ElegooLink_hpp_

#include "libslic3r/PrinterNetworkInfo.hpp"
#include "Singleton.hpp"
#include <string>
#include <vector>
#include <mutex>

namespace Slic3r {

class ElegooLink : public Singleton<ElegooLink> {
    friend class Singleton<ElegooLink>;

public:
    ElegooLink();
    ~ElegooLink();

    bool addPrinter(const PrinterNetworkInfo& printerNetworkInfo);
    void removePrinter(const PrinterNetworkInfo& printerNetworkInfo);
    bool isPrinterConnected(const PrinterNetworkInfo& printerNetworkInfo);

    std::vector<PrinterNetworkInfo> discoverDevices();
    bool sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);
    bool sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params);


private:
    mutable std::mutex mPrinterListMutex;
    std::vector<PrinterNetworkInfo> mPrinterList;
};

} // namespace Slic3r

#endif 