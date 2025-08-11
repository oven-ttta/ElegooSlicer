#ifndef slic3r_ElegooNetwork_hpp_
#define slic3r_ElegooNetwork_hpp_

#include "PrinterNetwork.hpp"

namespace Slic3r {

class ElegooNetwork : public IPrinterNetwork
{
public:
    ElegooNetwork();
    virtual ~ElegooNetwork();

    virtual PrinterNetworkResult<PrinterNetworkInfo> addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected) override;
    virtual PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo) override;
    virtual PrinterNetworkResult<bool> disconnectFromPrinter(const std::string& printerId) override;
    virtual PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) override;
    virtual PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) override;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverDevices() override;
    virtual PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const PrinterNetworkInfo& printerNetworkInfo) override;
    virtual void close() override;
    virtual int getDeviceType(const PrinterNetworkInfo& printerNetworkInfo) override;
};

} // namespace Slic3r

#endif
