#ifndef slic3r_PrinterNetwork_hpp_
#define slic3r_PrinterNetwork_hpp_

#include <string>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include "libslic3r/PrinterNetworkInfo.hpp"

namespace Slic3r {

class IPrinterNetwork
{
public:
    IPrinterNetwork(const PrinterNetworkInfo& printerNetworkInfo) : mPrinterNetworkInfo(printerNetworkInfo) {}
    IPrinterNetwork()=delete;
    IPrinterNetwork(const IPrinterNetwork&)=delete;
    IPrinterNetwork& operator=(const IPrinterNetwork&)=delete;
    virtual ~IPrinterNetwork() = default;
    virtual PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter() = 0;
    virtual PrinterNetworkResult<bool> disconnectFromPrinter() = 0;
    virtual PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkParams& params) = 0;
    virtual PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkParams& params) = 0;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverDevices() = 0;   
    virtual PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo() = 0;
    virtual void close() = 0;
    virtual int getDeviceType() = 0;

    const PrinterNetworkInfo& getPrinterNetworkInfo() const { return mPrinterNetworkInfo; }

protected:
    PrinterNetworkInfo mPrinterNetworkInfo;
    // if 1 to 1, implement http websocket etc. network management here
    // if ElegooLink, just redirect the interface

};

class PrinterNetworkFactory
{
public:
    static std::shared_ptr<IPrinterNetwork> createNetwork(const PrinterNetworkInfo& printerNetworkInfo);
};

} // namespace Slic3r

#endif
