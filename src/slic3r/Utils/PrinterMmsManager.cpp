#include "PrinterMmsManager.hpp"
#include "slic3r/Utils/PrinterManager.hpp"
#include <nlohmann/json.hpp>
#include <wx/colour.h>
#include "slic3r/GUI/MainFrame.hpp"

namespace Slic3r {

struct ColorRange
{
    int         r1, g1, b1;
    int         r2, g2, b2;
    std::string name;
    std::string hex_right;
};

static void hexToRgb(const std::string& hex, int& r, int& g, int& b)
{
    std::string h = hex;
    if (h[0] == '#')
        h = h.substr(1);
    if (h.length() == 6) {
        r = std::stoi(h.substr(0, 2), nullptr, 16);
        g = std::stoi(h.substr(2, 2), nullptr, 16);
        b = std::stoi(h.substr(4, 2), nullptr, 16);
    } else {
        r = g = b = 0;
    }
}

static const std::vector<ColorRange> colorRanges = {
    {230, 230, 230, 255, 255, 255, "white", "#FFFFFF"},             // #E6E6E6 ~ #FFFFFF
    {225, 202, 26, 255, 250, 246, "yellow", "#FFFAF6"},             // #E1CA1A ~ #FFFAF6
    {179, 204, 82, 255, 254, 162, "light_yellow_green", "#FFFEA2"}, // #B3CC52 ~ #FFFEA2
    {0, 164, 18, 49, 244, 98, "green", "#31F462"},                  // #00A412 ~ #31F462
    {0, 79, 31, 47, 159, 111, "dark_green", "#2F9F6F"},             // #004F1F ~ #2F9F6F
    {0, 58, 91, 51, 138, 171, "blue_green", "#338AAB"},             // #003A5B ~ #338AAB
    {0, 186, 120, 51, 255, 200, "cyan_green", "#33FFC8"},           // #00BA78 ~ #33FFC8
    {76, 177, 203, 156, 255, 254, "sky_blue", "#9CFFFE"},           // #4CB1CB ~ #9CFFFE
    {32, 127, 210, 112, 207, 255, "light_blue", "#70CFFF"},         // #207FD2 ~ #70CFFF
    {0, 40, 183, 80, 120, 255, "dark_blue", "#5078FF"},             // #0028B7 ~ #5078FF
    {27, 8, 97, 107, 88, 177, "purple", "#6B58B1"},                 // #1B0861 ~ #6B58B1
    {120, 19, 207, 200, 99, 255, "light_purple", "#C863FF"},        // #7813CF ~ #C863FF
    {203, 7, 208, 255, 87, 255, "pink_purple", "#FF57FF"},          // #CB07D0 ~ #FF57FF
    {172, 137, 181, 252, 217, 255, "light_pink_purple", "#FCD9FF"}, // #AC89B5 ~ #FCD9FF
    {209, 53, 79, 255, 133, 159, "pink", "#FF859F"},                // #D1354F ~ #FF859F
    {215, 0, 0, 255, 74, 73, "red", "#FF4A49"},                     // #D70000 ~ #FF4A49
    {84, 36, 0, 164, 116, 40, "brown", "#A47428"},                  // #542400 ~ #A47428
    {208, 101, 14, 255, 181, 94, "orange", "#FFB55E"},              // #D0650E ~ #FFB55E
    {212, 195, 175, 255, 255, 254, "beige", "#FFFFFF"},             // #D4C3AF ~ #FFFFFF
    {170, 157, 123, 250, 237, 203, "light_brown", "#FAEDCB"},       // #AA9D7B ~ #FAEDCB
    {135, 80, 10, 215, 160, 90, "dark_brown", "#D7A05A"},           // #87500A ~ #D7A05A
    {89, 89, 89, 153, 153, 153, "dark_gray", "#999999"},            // #595959 ~ #999999
    {153, 153, 153, 230, 230, 230, "light_gray", "#E6E6E6"},        // #999999 ~ #E6E6E6
    {0, 0, 0, 89, 89, 89, "black", "#595959"}                       // #000000 ~ #595959
};

static std::string getStandardColor(const std::string& hex)
{
    int r, g, b;
    hexToRgb(hex, r, g, b);
    for (const auto& range : colorRanges) {
        if (r >= range.r1 && r <= range.r2 && g >= range.g1 && g <= range.g2 && b >= range.b1 && b <= range.b2) {
            return range.hex_right;
        }
    }
    return "#000000";
}

PrinterMms PrinterMmsManager::convertJsonToPrinterMms(const nlohmann::json& json)
{
    PrinterMms mms;
    mms.mmsId       = json["mmsId"];
    mms.temperature = json["temperature"];
    mms.humidity    = json["humidity"];
    mms.connected   = json["connected"];
    for (auto& tray : json["trayList"]) {
        mms.trayList.push_back(convertJsonToPrinterMmsTray(tray));
    }
    return mms;
}

nlohmann::json PrinterMmsManager::convertPrinterMmsToJson(const PrinterMms& mms)
{
    nlohmann::json json = nlohmann::json::object();
    json["mmsId"]       = mms.mmsId;
    json["temperature"] = mms.temperature;
    json["humidity"]    = mms.humidity;
    json["connected"]   = mms.connected;
    nlohmann::json trayList = nlohmann::json::array();
    for (auto& tray : mms.trayList) {
        nlohmann::json trayJson = convertPrinterMmsTrayToJson(tray);
        trayList.push_back(trayJson);
    }
    json["trayList"] = trayList;
    return json;
}

nlohmann::json PrinterMmsManager::convertPrinterMmsTrayToJson(const PrinterMmsTray& tray)
{
    nlohmann::json json      = nlohmann::json::object();
    json["mmsId"]            = tray.mmsId;
    json["trayId"]           = tray.trayId;
    json["trayIndex"]        = tray.trayIndex;
    json["settingId"]        = tray.settingId;
    json["filamentId"]       = tray.filamentId;
    json["from"]             = tray.from;
    json["vendor"]           = tray.vendor;
    json["serialNumber"]     = tray.serialNumber;
    json["filamentType"]     = tray.filamentType;
    json["filamentName"]     = tray.filamentName;
    json["filamentColor"]    = tray.filamentColor;
    json["filamentDiameter"] = tray.filamentDiameter;
    json["minNozzleTemp"]    = tray.minNozzleTemp;
    json["maxNozzleTemp"]    = tray.maxNozzleTemp;
    json["minBedTemp"]       = tray.minBedTemp;
    json["maxBedTemp"]       = tray.maxBedTemp;
    json["status"]           = tray.status;
    return json;
}

PrinterMmsTray PrinterMmsManager::convertJsonToPrinterMmsTray(const nlohmann::json& json)
{
    PrinterMmsTray tray;
    tray.mmsId            = json["mmsId"];
    tray.trayId           = json["trayId"];
    tray.trayIndex        = json["trayIndex"];
    tray.settingId        = json["settingId"];
    tray.filamentId       = json["filamentId"];
    tray.from             = json["from"];
    tray.vendor           = json["vendor"];
    tray.serialNumber     = json["serialNumber"];
    tray.filamentType     = json["filamentType"];
    tray.filamentName     = json["filamentName"];
    tray.filamentColor    = json["filamentColor"];
    tray.filamentDiameter = json["filamentDiameter"];
    tray.minNozzleTemp    = json["minNozzleTemp"];
    tray.maxNozzleTemp    = json["maxNozzleTemp"];
    tray.minBedTemp       = json["minBedTemp"];
    tray.maxBedTemp       = json["maxBedTemp"];
    tray.status           = json["status"];
    return tray;
}

nlohmann::json PrinterMmsManager::convertPrinterMmsGroupToJson(const PrinterMmsGroup& mmsGroup)
{
    nlohmann::json json          = nlohmann::json::object();
    json["mmsConnected"]         = mmsGroup.mmsConnected;
    json["mmsType"]              = mmsGroup.mmsType;
    json["mmsConnectNum"]        = mmsGroup.mmsConnectNum;
    json["nozzleFilamentStatus"] = mmsGroup.nozzleFilamentStatus;
    json["activeMmsId"]          = mmsGroup.activeMmsId;
    json["activeTrayId"]         = mmsGroup.activeTrayId;
    json["autoRefill"]           = mmsGroup.autoRefill;
    json["vtTray"]               = convertPrinterMmsTrayToJson(mmsGroup.vtTray);
    nlohmann::json mmsList = nlohmann::json::array();
    for (auto& mms : mmsGroup.mmsList) {
        nlohmann::json mmsJson = convertPrinterMmsToJson(mms);
        mmsList.push_back(mmsJson);
    }
    json["mmsList"] = mmsList;
    return json;
}

PrinterMmsGroup PrinterMmsManager::convertJsonToPrinterMmsGroup(const nlohmann::json& json)
{
    PrinterMmsGroup mmsGroup;
    mmsGroup.mmsConnected         = json["mmsConnected"];
    mmsGroup.mmsType              = json["mmsType"];
    mmsGroup.mmsConnectNum        = json["mmsConnectNum"];
    mmsGroup.nozzleFilamentStatus = json["nozzleFilamentStatus"];
    mmsGroup.activeMmsId          = json["activeMmsId"];
    mmsGroup.activeTrayId         = json["activeTrayId"];
    mmsGroup.autoRefill           = json["autoRefill"];
    mmsGroup.vtTray               = convertJsonToPrinterMmsTray(json["vtTray"]);
    for (auto& mms : json["mmsList"]) {
        mmsGroup.mmsList.push_back(convertJsonToPrinterMms(mms));
    }
    return mmsGroup;
}

nlohmann::json PrinterMmsManager::convertPrintFilamentMmsMappingToJson(const PrintFilamentMmsMapping& printFilamentMmsMapping)
{
    nlohmann::json json       = nlohmann::json::object();
    json["filamentId"]        = printFilamentMmsMapping.filamentId;
    json["filamentName"]      = printFilamentMmsMapping.filamentName;
    json["filamentColor"]     = printFilamentMmsMapping.filamentColor;
    json["filamentType"]      = printFilamentMmsMapping.filamentType;
    std::ostringstream weightStream, densityStream;
    weightStream << std::fixed << std::setprecision(2) << printFilamentMmsMapping.filamentWeight;
    densityStream << std::fixed << std::setprecision(2) << printFilamentMmsMapping.filamentDensity;
    json["filamentWeight"]    = weightStream.str();
    json["filamentDensity"]   = densityStream.str();
    json["mappedMmsFilament"] = convertPrinterMmsTrayToJson(printFilamentMmsMapping.mappedMmsFilament);
    return json;
}

PrintFilamentMmsMapping PrinterMmsManager::convertJsonToPrintFilamentMmsMapping(const nlohmann::json& json)
{
    PrintFilamentMmsMapping printFilamentMmsMapping;
    printFilamentMmsMapping.filamentId        = json["filamentId"];
    printFilamentMmsMapping.filamentName      = json["filamentName"];
    printFilamentMmsMapping.filamentColor     = json["filamentColor"];
    printFilamentMmsMapping.filamentType      = json["filamentType"];
    printFilamentMmsMapping.filamentWeight    = std::stof(json["filamentWeight"].get<std::string>());
    printFilamentMmsMapping.filamentDensity   = std::stof(json["filamentDensity"].get<std::string>());
    printFilamentMmsMapping.mappedMmsFilament = convertJsonToPrinterMmsTray(json["mappedMmsFilament"]);
    return printFilamentMmsMapping;
}

PrinterMmsManager::PrinterMmsManager() {}

PrinterMmsManager::~PrinterMmsManager() {}

PrinterMmsGroup PrinterMmsManager::getPrinterMmsInfo(const std::string& printerId)
{
    return PrinterManager::getInstance()->getPrinterMmsInfo(printerId);
}

void PrinterMmsManager::getFilamentMmsMapping(std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping, const PrinterMmsGroup& mmsGroup)
 {
    AppConfig* app_config = wxGetApp().app_config;
    for (auto& printFilament : printFilamentMmsMapping) {
        std::string filamentStandardColor   = getStandardColor(printFilament.filamentColor);
        std::string key                     = printFilament.filamentId + "_" + filamentStandardColor;
        std::string mmsMappingFilamentInfo  = app_config->get(CONFIG_KEY_MMS_FILAMENT_MAPPING, key);
        std::string mmsMappingFilamentName  = "";
        std::string mmsMappingFilamentColor = "";
        bool        isMapped                = false;
        // already mapped
        if (!mmsMappingFilamentInfo.empty()) {
            std::vector<std::string> mmsMappingFilamentInfoList;
            boost::split(mmsMappingFilamentInfoList, mmsMappingFilamentInfo, boost::is_any_of("_"));
            if (mmsMappingFilamentInfoList.size() == 2) {
                mmsMappingFilamentName  = mmsMappingFilamentInfoList[0];
                mmsMappingFilamentColor = mmsMappingFilamentInfoList[1];
            }
        }
        if (!mmsMappingFilamentName.empty() && !mmsMappingFilamentColor.empty()) {
            for (auto& mms : mmsGroup.mmsList) {
                for (auto& tray : mms.trayList) {
                    if (tray.filamentName == mmsMappingFilamentName && tray.filamentColor == mmsMappingFilamentColor) {
                        printFilament.mappedMmsFilament.trayId           = tray.trayId;
                        printFilament.mappedMmsFilament.trayIndex        = tray.trayIndex;
                        printFilament.mappedMmsFilament.mmsId            = mms.mmsId;
                        printFilament.mappedMmsFilament.filamentType     = tray.filamentType;
                        printFilament.mappedMmsFilament.filamentName     = tray.filamentName;
                        printFilament.mappedMmsFilament.filamentColor    = tray.filamentColor;
                        printFilament.mappedMmsFilament.filamentDiameter = tray.filamentDiameter;
                        printFilament.mappedMmsFilament.minNozzleTemp    = tray.minNozzleTemp;
                        printFilament.mappedMmsFilament.maxNozzleTemp    = tray.maxNozzleTemp;
                        printFilament.mappedMmsFilament.minBedTemp       = tray.minBedTemp;
                        printFilament.mappedMmsFilament.maxBedTemp       = tray.maxBedTemp;
                        printFilament.mappedMmsFilament.status           = tray.status;
                        isMapped                                         = true;
                        break;
                    }
                }
            }

            if (isMapped) {
                continue;
            }
            // not mapped or mapped filament not exist
            for (auto& mms : mmsGroup.mmsList) {
                for (auto& tray : mms.trayList) {
                    if (tray.filamentName == printFilament.filamentName && tray.filamentColor == filamentStandardColor) {
                        printFilament.mappedMmsFilament.trayIndex        = tray.trayIndex;
                        printFilament.mappedMmsFilament.trayId           = tray.trayId;
                        printFilament.mappedMmsFilament.mmsId            = mms.mmsId;
                        printFilament.mappedMmsFilament.filamentName     = tray.filamentName;
                        printFilament.mappedMmsFilament.filamentColor    = tray.filamentColor;
                        printFilament.mappedMmsFilament.filamentType     = tray.filamentType;
                        printFilament.mappedMmsFilament.filamentDiameter = tray.filamentDiameter;
                        printFilament.mappedMmsFilament.minNozzleTemp    = tray.minNozzleTemp;
                        printFilament.mappedMmsFilament.maxNozzleTemp    = tray.maxNozzleTemp;
                        printFilament.mappedMmsFilament.minBedTemp       = tray.minBedTemp;
                        printFilament.mappedMmsFilament.maxBedTemp       = tray.maxBedTemp;
                        printFilament.mappedMmsFilament.status           = tray.status;
                        isMapped                                         = true;
                        break;
                    }
                }
            }
        }
    }
}

void PrinterMmsManager::saveFilamentMmsMapping(std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping)
{
    AppConfig* app_config = wxGetApp().app_config;
    app_config->clear_section("mms_filament_mapping");
    for (auto& printFilament : printFilamentMmsMapping) {
        if (printFilament.mappedMmsFilament.trayIndex.empty() || printFilament.mappedMmsFilament.trayId.empty() ||
            printFilament.mappedMmsFilament.mmsId.empty() || printFilament.mappedMmsFilament.filamentName.empty() ||
            printFilament.mappedMmsFilament.filamentColor.empty()) {
            continue;
        }
        std::string filamentStandardColor  = getStandardColor(printFilament.filamentColor);
        std::string key                    = printFilament.filamentId + "_" + filamentStandardColor;
        std::string mmsMappingFilamentInfo = printFilament.mappedMmsFilament.filamentName + "_" +
                                             printFilament.mappedMmsFilament.filamentColor;
        app_config->set("mms_filament_mapping", key, mmsMappingFilamentInfo);
    }
}

} // namespace Slic3r
