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
    virtual ~IPrinterNetwork() = default;

    virtual PrinterNetworkResult<PrinterNetworkInfo> addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected) = 0;
    virtual PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo) = 0;
    virtual PrinterNetworkResult<bool> disconnectFromPrinter(const std::string& printerId) = 0;
    virtual PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) = 0;
    virtual PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) = 0;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverDevices() = 0;   
    virtual PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const PrinterNetworkInfo& printerNetworkInfo) = 0;
    virtual void close() = 0;
    virtual int getDeviceType(const PrinterNetworkInfo& printerNetworkInfo) = 0;
};

class PrinterNetworkFactory
{
public:
    static std::shared_ptr<IPrinterNetwork> createNetwork(const PrintHostType hostType);
};

} // namespace Slic3r

#endif
