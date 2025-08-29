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
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters() = 0;   
    virtual PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo() = 0;
    virtual PrinterNetworkResult<PrinterNetworkInfo> getPrinterAttributes() = 0;

    const PrinterNetworkInfo& getPrinterNetworkInfo() const { return mPrinterNetworkInfo; }

    // init and uninit the all network
    static void init();
    static void uninit();

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
