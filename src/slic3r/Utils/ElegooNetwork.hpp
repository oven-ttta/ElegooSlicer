#ifndef slic3r_ElegooNetwork_hpp_
#define slic3r_ElegooNetwork_hpp_

#include "PrinterNetwork.hpp"

namespace Slic3r {

class ElegooNetwork : public IPrinterNetwork
{
public:
    ElegooNetwork();
    virtual ~ElegooNetwork();

    virtual PrinterNetworkResult<bool> addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected) override;
    virtual PrinterNetworkResult<bool> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo) override;
    virtual PrinterNetworkResult<bool> disconnectFromPrinter(const std::string& printerId) override;
    virtual PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) override;
    virtual PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) override;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverDevices() override;
};

} // namespace Slic3r

#endif
