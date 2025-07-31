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
    IPrinterNetwork(const PrinterNetworkInfo& printerNetworkInfo);
    virtual ~IPrinterNetwork() = default;

    virtual bool                     connect()                                         = 0;
    virtual void                     disconnect()                                      = 0;
    virtual bool                     isConnected() const                               = 0;
    virtual bool                     sendPrintTask(const PrinterNetworkParams& params) = 0;
    virtual bool                     sendPrintFile(const PrinterNetworkParams& params) = 0;
    virtual std::vector<PrinterNetworkInfo> discoverDevices()                          = 0;

    IPrinterNetwork()                                  = delete;
    IPrinterNetwork& operator=(const IPrinterNetwork&) = delete;

protected:
    PrinterNetworkInfo m_printerNetworkInfo;
};

class PrinterNetworkFactory
{
public:
    static std::unique_ptr<IPrinterNetwork> createNetwork(const PrinterNetworkInfo& printerNetworkInfo, const PrintHostType hostType);
};
} // namespace Slic3r

#endif
