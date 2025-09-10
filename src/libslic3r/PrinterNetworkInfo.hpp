#ifndef slic3r_PrinterNetworkInfo_hpp_
#define slic3r_PrinterNetworkInfo_hpp_


#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include "PrinterNetworkResult.hpp"

namespace Slic3r {

enum PrinterConnectStatus {
    PRINTER_CONNECT_STATUS_DISCONNECTED = 0,
    PRINTER_CONNECT_STATUS_CONNECTED    = 1,
};
enum PrinterStatus {
    PRINTER_STATUS_OFFLINE            = -1,
    PRINTER_STATUS_IDLE               = 0,
    PRINTER_STATUS_PRINTING           = 1,
    PRINTER_STATUS_PAUSED             = 2,
    PRINTER_STATUS_PAUSING            = 3,
    PRINTER_STATUS_CANCELED           = 4,
    PRINTER_STATUS_SELF_CHECKING      = 5,  // Self-checking
    PRINTER_STATUS_AUTO_LEVELING      = 6,  // Auto-leveling
    PRINTER_STATUS_PID_CALIBRATING    = 7,  // PID calibrating
    PRINTER_STATUS_RESONANCE_TESTING  = 8,  // Resonance testing
    PRINTER_STATUS_UPDATING           = 9,  // Updating
    PRINTER_STATUS_FILE_COPYING       = 10, // File copying
    PRINTER_STATUS_FILE_TRANSFERRING  = 11, // File transferring
    PRINTER_STATUS_HOMING             = 12, // Homing
    PRINTER_STATUS_PREHEATING         = 13, // Preheating
    PRINTER_STATUS_FILAMENT_OPERATING = 14, // Filament operating
    PRINTER_STATUS_EXTRUDER_OPERATING = 15, // Extruder operating
    PRINTER_STATUS_PRINT_COMPLETED    = 16, // Print completed
    PRINTER_STATUS_RFID_RECOGNIZING   = 17, // RFID recognizing

    PRINTER_STATUS_BUSY   = 998,  // Device busy
    PRINTER_STATUS_ERROR   = 999,  // Device exception status
    PRINTER_STATUS_ID_NOT_MATCH = 1000, // local mainboard or serial number not match with the remote printer
    PRINTER_STATUS_AUTH_ERROR = 1001, // local auth mode not match with the remote printer
    PRINTER_STATUS_UNKNOWN = 10000, // Unknown status
};

enum TrayStatus {
    TRAY_STATUS_UNKNOWN = -1,
    TRAY_STATUS_DISCONNECTED = 0, // disconnected
    TRAY_STATUS_PRELOADED = 1, // preloaded
    TRAY_STATUS_PRELOADED_UNKNOWN_FILAMENT = 2, // preloaded unknown filament info
    TRAY_STATUS_LOADED = 3, // loaded
    TRAY_STATUS_LOADED_UNKNOWN_FILAMENT = 4, // loaded unknown filament info
    TRAY_STATUS_ERROR = 999, // error
};
struct PrinterPrintTask
{
    std::string taskId;
    std::string fileName;
    int         totalTime{0};
    int         currentTime{0};
    int         estimatedTime{0};
    int         progress{0};
};

#define PRINTER_NETWORK_EXTRA_INFO_KEY_HOST "deviceUi"
#define PRINTER_NETWORK_EXTRA_INFO_KEY_PORT "httpsCaFile"
#define PRINTER_NETWORK_EXTRA_INFO_KEY_VENDOR "apiKey"

struct PrinterMmsTray
{
    std::string trayId;
    std::string mmsId;
    std::string trayName;
    std::string settingId;
    std::string filamentId;
    std::string from;
    std::string vendor;
    int         serialNumber;
    std::string filamentType;
    std::string filamentName;
    std::string filamentColor;
    std::string filamentDiameter;
    double      minNozzleTemp;
    double      maxNozzleTemp;
    double      minBedTemp;
    double      maxBedTemp;
    TrayStatus  status{TRAY_STATUS_UNKNOWN};
};

struct PrinterMms
{
    std::string                 mmsId;
    std::string                 mmsName;
    std::string                 mmsModel;
    double                      temperature;
    int                         humidity;
    bool                        connected;
    std::vector<PrinterMmsTray> trayList;
};

// Multi-Material System
struct PrinterMmsGroup
{
    int                     connectNum;
    bool                    connected;
    std::string             activeMmsId;
    std::string             activeTrayId;
    bool                    autoRefill;
    std::string             mmsSystemName;
    std::vector<PrinterMms> mmsList;
    PrinterMmsTray          vtTray;
};

struct PrintFilamentMmsMapping
{
    int index;
    std::string filamentId;
    std::string settingId;
    std::string filamentName;
    std::string filamentAlias;
    std::string filamentColor;
    std::string filamentType;
    float       filamentWeight;
    float       filamentDensity;
    //print mapping mms filament
    PrinterMmsTray mappedMmsFilament;
};

struct PrintCapabilities
{
    bool supportsAutoBedLeveling = false;    // Supports auto bed leveling
    bool supportsTimeLapse = false;          // Supports time-lapse printing
    bool supportsHeatedBedSwitching = false; // Supports heated bed switching
    bool supportsFilamentMapping = false;    // Supports filament mapping
};

struct SystemCapabilities
{
    bool canSetPrinterName = false;     // Supports setting machine name
    bool canGetDiskInfo = false;        // Supports getting disk information
    bool supportsMultiFilament = false; // Supports multi-filament printing
};

enum PrinterAuthMode {
    PRINTER_AUTH_MODE_NONE = 0,
    PRINTER_AUTH_MODE_PASSWORD = 1,
    PRINTER_AUTH_MODE_ACCESS_CODE = 2,
    PRINTER_AUTH_MODE_PIN_CODE = 3,
};
struct PrinterNetworkInfo
{
    std::string printerName;
    std::string printerId;
    std::string host;
    int         port{0};
    std::string vendor;
    std::string printerModel;
    std::string protocolVersion;
    std::string firmwareVersion;
    std::string hostType;
    std::string mainboardId;
    std::string serialNumber;
    std::string username;
    std::string password;
    PrinterAuthMode authMode{PRINTER_AUTH_MODE_NONE};
    std::string token;
    std::string accessCode;
    std::string pinCode;
    std::string webUrl;
    std::string connectionUrl;
    bool        isPhysicalPrinter{false};
    uint64_t    addTime{0};
    uint64_t    modifyTime{0};
    uint64_t    lastActiveTime{0};
    PrintCapabilities  printCapabilities;
    SystemCapabilities   systemCapabilities;
    std::string extraInfo{"{}"}; // json string
    // not save to file
    PrinterPrintTask     printTask;
    PrinterConnectStatus connectStatus{PRINTER_CONNECT_STATUS_DISCONNECTED};
    PrinterStatus        printerStatus{PRINTER_STATUS_IDLE};
};

struct PrinterNetworkParams
{
    std::string printerId;
    std::string filePath;
    std::string fileName;
    int         bedType{0};
    bool        timeLapse{false};
    bool        heatedBedLeveling{false};
    bool        autoRefill{false};
    bool        uploadAndStartPrint{false};
    std::vector<PrintFilamentMmsMapping> filamentMmsMappingList;

    std::function<void(const uint64_t uploadedBytes, const uint64_t totalBytes, bool& cancel)> uploadProgressFn;
    std::function<void(const std::string& errorMsg)>                                           errorFn;
};

using PrinterConnectStatusFn = std::function<void(const std::string& printerId, const PrinterConnectStatus& status)>;
using PrinterStatusFn        = std::function<void(const std::string& printerId, const PrinterStatus& status)>;
using PrinterPrintTaskFn     = std::function<void(const std::string& printerId, const PrinterPrintTask& printTask)>;
using PrinterAttributesFn    = std::function<void(const std::string& printerId, const PrinterNetworkInfo& printerInfo)>;

PrinterNetworkInfo convertJsonToPrinterNetworkInfo(const nlohmann::json& json);
nlohmann::json convertPrinterNetworkInfoToJson(const PrinterNetworkInfo& printerNetworkInfo);
nlohmann::json convertPrinterMmsTrayToJson(const PrinterMmsTray& tray);
PrinterMmsTray convertJsonToPrinterMmsTray(const nlohmann::json& json);
nlohmann::json convertPrinterMmsToJson(const PrinterMms& mms);
PrinterMms convertJsonToPrinterMms(const nlohmann::json& json);
nlohmann::json convertPrinterMmsGroupToJson(const PrinterMmsGroup& mmsGroup);
PrinterMmsGroup convertJsonToPrinterMmsGroup(const nlohmann::json& json);
nlohmann::json convertPrintFilamentMmsMappingToJson(const PrintFilamentMmsMapping& printFilamentMmsMapping);
PrintFilamentMmsMapping convertJsonToPrintFilamentMmsMapping(const nlohmann::json& json);

} // namespace Slic3r

#endif