#ifndef slic3r_PrinterNetworkInfo_hpp_
#define slic3r_PrinterNetworkInfo_hpp_

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace Slic3r {


enum PrinterNetworkStatus {
    PRINTER_NETWORK_STATUS_UNKNOWN = -1,
    PRINTER_NETWORK_STATUS_CONNECTED = 0,
    PRINTER_NETWORK_STATUS_DISCONNECTED = 1,
    PRINTER_NETWORK_STATUS_PRINTING = 2,
    PRINTER_NETWORK_STATUS_PAUSED = 3,
    PRINTER_NETWORK_STATUS_CANCELLED = 4,
    PRINTER_NETWORK_STATUS_ERROR = 5,
};


struct PrinterNetworkInfo
{
    std::string id;
    std::string name;
    std::string ip;
    int port{0};
    std::string vendor;
    std::string machineName;
    std::string machineModel;
    std::string protocolVersion;
    std::string firmwareVersion;
    std::string deviceId;
    int deviceType{0};
    std::string serialNumber;
    std::string webUrl;
    std::string connectionUrl;
    bool isPhysicalPrinter{false};
    uint64_t addTime{0};
    uint64_t modifyTime{0};
    uint64_t lastActiveTime{0};

    template<class Archive>
    void serialize(Archive& ar) {
        ar(id, name, ip, port, vendor, machineName,
           machineModel, protocolVersion, firmwareVersion, deviceId,
           deviceType, serialNumber, webUrl, connectionUrl,
           isPhysicalPrinter, addTime, modifyTime, lastActiveTime);
    }
};

struct PrinterNetworkPrintTask {
    std::string taskId;
    std::string fileName;
    int totalTime{0};
    int currentTime{0};
    int estimatedTime{0};
    int progress{0};
};

struct PrinterNetworkParams
{
    PrinterNetworkInfo printerNetworkInfo;
    std::string filePath;
    std::string fileName;
    std::string printerId;
    int bedType{0};
    bool timeLapse{false};
    bool heatedBedLeveling{false};
    bool autoRefill{false};
    bool uploadAndStartPrint{false};

    std::function<void(const uint64_t uploadedBytes, const uint64_t totalBytes, bool& cancel)> uploadProgressFn;
    std::function<void(const std::string& errorMsg)> errorFn;
};


using PrinterStatusFn = std::function<void(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkPrintTask& printTask)>;
using PrintTaskFn = std::function<void(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkPrintTask& printTask)>;

} // namespace Slic3r


#endif