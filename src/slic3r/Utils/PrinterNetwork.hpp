#ifndef slic3r_PrinterNetwork_hpp_
#define slic3r_PrinterNetwork_hpp_

#include <string>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <cereal/archives/binary.hpp>
#include "slic3r/Utils/PrintHost.hpp"

namespace Slic3r {
struct PrinterInfo
{
    std::string id;
    std::string name;
    std::string ip;
    std::string port;
    std::string vendor;
    std::string machineName;
    std::string machineModel;
    std::string protocolVersion;
    std::string firmwareVersion;
    std::string deviceId;
    std::string deviceType;
    std::string serialNumber;
    std::string webUrl;
    std::string connectionUrl;
    bool        isPhysicalPrinter;

    template<class Archive> void serialize(Archive& ar)
    {
        ar(id, name, ip, port, vendor, machineName,
           machineModel, protocolVersion, firmwareVersion, deviceId, deviceType,
           serialNumber, webUrl, connectionUrl, isPhysicalPrinter);
    }
};

struct PrinterNetworkParams
{
    PrintHostUpload       uploadData;
    PrintHost::ProgressFn progressFn;
    PrintHost::ErrorFn    errorFn;
    PrintHost::InfoFn     infoFn;
};

class IPrinterNetwork
{
public:
    IPrinterNetwork(const PrinterInfo& printerInfo);
    virtual ~IPrinterNetwork() = default;

    virtual bool                     connect()                                         = 0;
    virtual void                     disconnect()                                      = 0;
    virtual bool                     isConnected() const                               = 0;
    virtual bool                     sendPrintTask(const PrinterNetworkParams& params) = 0;
    virtual bool                     sendPrintFile(const PrinterNetworkParams& params) = 0;
    virtual std::vector<PrinterInfo> discoverDevices()                                 = 0;

    IPrinterNetwork()                                  = delete;
    IPrinterNetwork& operator=(const IPrinterNetwork&) = delete;

protected:
    PrinterInfo m_printerInfo;
};

class PrinterNetworkFactory
{
public:
    static std::unique_ptr<IPrinterNetwork> createNetwork(const PrinterInfo& printerInfo, const PrintHostType hostType);
};
} // namespace Slic3r

#endif
