#include "PrinterMmsManager.hpp"
#include "slic3r/Utils/Elegoo/PrinterManager.hpp"
#include <nlohmann/json.hpp>
#include <wx/colour.h>
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/filesystem.hpp>
#include <mutex>
#include <cmath>
#include <algorithm>

namespace Slic3r {

struct StandardColor
{
    std::string colorName;
    std::string colorHex;
};

struct RgbColor {
    int r, g, b;
    RgbColor(int red = 0, int green = 0, int blue = 0) : r(red), g(green), b(blue) {}
};

static RgbColor hexToRgb(const std::string& hex)
{
    std::string h = hex;
    if (!h.empty() && h[0] == '#')
        h = h.substr(1);
    
    if (h.length() == 6) {
        int r = std::stoi(h.substr(0, 2), nullptr, 16);
        int g = std::stoi(h.substr(2, 2), nullptr, 16);
        int b = std::stoi(h.substr(4, 2), nullptr, 16);
        return RgbColor(r, g, b);
    }
    
    return RgbColor(0, 0, 0);
}

struct XyzColor {
    double x, y, z;
    XyzColor(double xVal = 0, double yVal = 0, double zVal = 0) : x(xVal), y(yVal), z(zVal) {}
};

static XyzColor rgbToXyz(const RgbColor& rgb)
{
    double rNorm = rgb.r / 255.0;
    double gNorm = rgb.g / 255.0;
    double bNorm = rgb.b / 255.0;
    
    // Apply gamma correction
    rNorm = (rNorm > 0.04045) ? pow((rNorm + 0.055) / 1.055, 2.4) : rNorm / 12.92;
    gNorm = (gNorm > 0.04045) ? pow((gNorm + 0.055) / 1.055, 2.4) : gNorm / 12.92;
    bNorm = (bNorm > 0.04045) ? pow((bNorm + 0.055) / 1.055, 2.4) : bNorm / 12.92;
    
    // Convert to XYZ using sRGB matrix
    double x = rNorm * 0.4124564 + gNorm * 0.3575761 + bNorm * 0.1804375;
    double y = rNorm * 0.2126729 + gNorm * 0.7151522 + bNorm * 0.0721750;
    double z = rNorm * 0.0193339 + gNorm * 0.1191920 + bNorm * 0.9503041;
    
    // Scale by 100
    return XyzColor(x * 100, y * 100, z * 100);
}

struct LabColor {
    double l, a, b;
    LabColor(double lVal = 0, double aVal = 0, double bVal = 0) : l(lVal), a(aVal), b(bVal) {}
};

static LabColor xyzToLab(const XyzColor& xyz)
{
    // D65 illuminant
    const double xn = 95.047;
    const double yn = 100.000;
    const double zn = 108.883;
    
    double fx = (xyz.x / xn > 0.008856) ? pow(xyz.x / xn, 1.0/3.0) : (7.787 * xyz.x / xn + 16.0/116.0);
    double fy = (xyz.y / yn > 0.008856) ? pow(xyz.y / yn, 1.0/3.0) : (7.787 * xyz.y / yn + 16.0/116.0);
    double fz = (xyz.z / zn > 0.008856) ? pow(xyz.z / zn, 1.0/3.0) : (7.787 * xyz.z / zn + 16.0/116.0);
    
    double l = 116 * fy - 16;
    double a = 500 * (fx - fy);
    double b = 200 * (fy - fz);
    
    return LabColor(l, a, b);
}

// Delta E 2000 calculation
static double deltaE2000(const LabColor& lab1, const LabColor& lab2)
{
    // Calculate C* and h
    double c1 = sqrt(lab1.a * lab1.a + lab1.b * lab1.b);
    double c2 = sqrt(lab2.a * lab2.a + lab2.b * lab2.b);
    double cBar = (c1 + c2) / 2.0;
    
    // Calculate G factor
    double g = 0.5 * (1 - sqrt(pow(cBar, 7) / (pow(cBar, 7) + pow(25, 7))));
    
    // Calculate a' and C'
    double a1Prime = (1 + g) * lab1.a;
    double a2Prime = (1 + g) * lab2.a;
    double c1Prime = sqrt(a1Prime * a1Prime + lab1.b * lab1.b);
    double c2Prime = sqrt(a2Prime * a2Prime + lab2.b * lab2.b);
    
    // Calculate h'
    double h1Prime = atan2(lab1.b, a1Prime) * 180.0 / M_PI;
    double h2Prime = atan2(lab2.b, a2Prime) * 180.0 / M_PI;
    if (h1Prime < 0) h1Prime += 360;
    if (h2Prime < 0) h2Prime += 360;
    
    // Calculate ΔL', ΔC', ΔH'
    double deltaL = lab2.l - lab1.l;
    double deltaC = c2Prime - c1Prime;
    double deltaH = h2Prime - h1Prime;
    if (deltaH > 180) deltaH -= 360;
    if (deltaH < -180) deltaH += 360;
    double deltaHPrime = 2 * sqrt(c1Prime * c2Prime) * sin(deltaH * M_PI / 360.0);
    
    // Calculate weighting functions
    double lBar = (lab1.l + lab2.l) / 2.0;
    double cBarPrime = (c1Prime + c2Prime) / 2.0;
    double hBarPrime = (h1Prime + h2Prime) / 2.0;
    if (abs(h1Prime - h2Prime) > 180) hBarPrime += 180;
    if (hBarPrime > 360) hBarPrime -= 360;
    
    double t = 1 - 0.17 * cos((hBarPrime - 30) * M_PI / 180.0) + 
               0.24 * cos((2 * hBarPrime) * M_PI / 180.0) +
               0.32 * cos((3 * hBarPrime + 6) * M_PI / 180.0) -
               0.20 * cos((4 * hBarPrime - 63) * M_PI / 180.0);
    
    double deltaTheta = 30 * exp(-pow((hBarPrime - 275) / 25, 2));
    double rC = 2 * sqrt(pow(cBarPrime, 7) / (pow(cBarPrime, 7) + pow(25, 7)));
    double sL = 1 + (0.015 * pow(lBar - 50, 2)) / sqrt(20 + pow(lBar - 50, 2));
    double sC = 1 + 0.045 * cBarPrime;
    double sH = 1 + 0.015 * cBarPrime * t;
    double rT = -sin(2 * deltaTheta * M_PI / 180.0) * rC;
    
    double kL = 1, kC = 1, kH = 1; // Standard observer
    
    double deltaE = sqrt(pow(deltaL / (kL * sL), 2) + 
                         pow(deltaC / (kC * sC), 2) + 
                         pow(deltaHPrime / (kH * sH), 2) + 
                         rT * (deltaC / (kC * sC)) * (deltaHPrime / (kH * sH)));
    
    return deltaE;
}

static const std::vector<StandardColor> standardColors = {{"white", "#FFFFFF"},
                                                          {"yellow", "#FFF242"},
                                                          {"light_yellow_green", "#DBF47A"},
                                                          {"green", "#09CC3A"},
                                                          {"dark_green", "#077747"},
                                                          {"blue_green", "#0B6283"},
                                                          {"cyan_green", "#0BE2A0"},
                                                          {"sky_blue", "#74D9F3"},
                                                          {"light_blue", "#48A7FA"},
                                                          {"dark_blue", "#2850DF"},
                                                          {"purple", "#433089"},
                                                          {"light_purple", "#A03BF7"},
                                                          {"pink_purple", "#F32FF8"},
                                                          {"light_pink_purple", "#D4B1DD"},
                                                          {"pink", "#F95D77"},
                                                          {"red", "#F72221"},
                                                          {"brown", "#7C4C00"},
                                                          {"orange", "#F88D36"},
                                                          {"beige", "#FCEBD7"},
                                                          {"light_brown", "#D2C5A3"},
                                                          {"dark_brown", "#AF7832"},
                                                          {"dark_gray", "#898989"},
                                                          {"light_gray", "#BCBCBC"},
                                                          {"black", "#000000"}};

// ΔE2000 (CIEDE2000) color match
// 0-1 barely noticeable to the human eye
// 1-2 noticeable only when compared closely
// 2-10 noticeable to the average person
// 11-49 noticeably different
// 50+ completely different colors

static StandardColor getStandardColor(const std::string& hex)
{
    // Convert input color to LAB
    RgbColor rgb = hexToRgb(hex);
    XyzColor xyz = rgbToXyz(rgb);
    LabColor lab = xyzToLab(xyz);
    
    double minDeltaE = 1000.0; // Large initial value
    StandardColor bestMatch = {"", ""};
    
    // Calculate ΔE2000 with each standard color
    for (const auto& color : standardColors) {
        RgbColor standardRgb = hexToRgb(color.colorHex);
        XyzColor standardXyz = rgbToXyz(standardRgb);
        LabColor standardLab = xyzToLab(standardXyz);
        
        // Calculate ΔE2000
        double deltaE = deltaE2000(lab, standardLab);
        
        // Find the closest match
        if (deltaE < minDeltaE) {
            minDeltaE = deltaE;
            bestMatch = color;
        }
    }
    
    // Return the closest match if ΔE < 50 (reasonable threshold)
    if (minDeltaE < 50.0) {
        return bestMatch;
    }
    
    // If no good match found, return empty
    return StandardColor{"", ""};
}

PrinterMmsManager::PrinterMmsManager() {}

PrinterMmsManager::~PrinterMmsManager() {}


// find mms tray filament id in system preset by filament name and type and compatible printers, 
// if not found, find filament id in orca generic preset by filament type
// when checking the name, pay attention to whether it contains the vendor name, space, etc.
// 1. find filament id in vendor preset, to upper case and add vendor name prefix if not contains
// 2. find filament id in vendor generic preset, to upper case and remove vendor name prefix if contains and add "GENERIC " prefix if not contains
// 3. find filament id in orca generic preset, to upper case and remove vendor name prefix if contains and add "GENERIC " prefix if not contains

void PrinterMmsManager::getMmsTrayFilamentId(const PrinterNetworkInfo& printerNetworkInfo, PrinterMmsGroup& mmsGroup)
{
    std::vector<double> currentProjectNozzleDiameters = {0.4};
    try {
        auto& app = GUI::wxGetApp();
        if(app.preset_bundle) {
            auto nozzleDiameterOpt = app.preset_bundle->printers.get_edited_preset().config.option<ConfigOptionFloats>("nozzle_diameter");
            if(nozzleDiameterOpt && !nozzleDiameterOpt->values.empty()) {
                currentProjectNozzleDiameters = nozzleDiameterOpt->values;
            }
        }
    } catch(...) {
    }
    
    PresetBundle vendorBundle;
    try {
        vendorBundle.load_vendor_configs_from_json((boost::filesystem::path(Slic3r::resources_dir()) / "profiles").string(),
                                                   printerNetworkInfo.vendor, PresetBundle::LoadSystem,
                                                   ForwardCompatibilitySubstitutionRule::EnableSilent, nullptr);                                        

    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "PrinterMmsManager::getMmsTrayFilamentId: get vendor configs failed: " << printerNetworkInfo.vendor << " " << e.what();
    }

    std::map<std::string, PrinterPresetInfo> printerPresetMap;
    for(const auto& printer : vendorBundle.printers) {
        if(!printer.is_system) continue;
        
        PrinterPresetInfo info;
        auto printerModelOpt = printer.config.option<ConfigOptionString>("printer_model");
        if(printerModelOpt) {
            info.printerModel = printerModelOpt->value;
        }
        
        auto nozzleDiameterOpt = printer.config.option<ConfigOptionFloats>("nozzle_diameter");
        if(nozzleDiameterOpt) {
            info.nozzleDiameters = nozzleDiameterOpt->values;
        }
        
        printerPresetMap[printer.name] = info;
    }

    auto vendorPresetMap = buildPresetFilamentMap(vendorBundle, printerNetworkInfo, printerPresetMap, currentProjectNozzleDiameters, false, true);
    auto genericPresetMap = buildPresetFilamentMap(vendorBundle, printerNetworkInfo, printerPresetMap, currentProjectNozzleDiameters, true, true);

    PresetBundle orcaFilamentLibraryBundle;
    try {
        orcaFilamentLibraryBundle.load_vendor_configs_from_json((boost::filesystem::path(Slic3r::resources_dir()) / "profiles").string(),
                                                   PresetBundle::ORCA_FILAMENT_LIBRARY, PresetBundle::LoadSystem,
                                                   ForwardCompatibilitySubstitutionRule::EnableSilent, nullptr);                                        

    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "PrinterMmsManager::getMmsTrayFilamentId: get orca filament library failed" << e.what();
    }
    
    auto orcaGenericPresetMap = buildPresetFilamentMap(orcaFilamentLibraryBundle, printerNetworkInfo, printerPresetMap, currentProjectNozzleDiameters, true, false);

    // match filament id in system preset to mms tray
    for(auto& mms : mmsGroup.mmsList) {
        for(auto& tray : mms.trayList) {
            if(!checkTrayIsReady(tray)) {
                continue;
            }
            if (tryMatchFilament(tray, vendorPresetMap, printerNetworkInfo, false))
                continue;  
                      
            // try match vendor generic preset
            if (tryMatchFilament(tray, genericPresetMap, printerNetworkInfo, true))
                continue;
       
            // try match orca generic preset
            if (tryMatchFilament(tray, orcaGenericPresetMap, printerNetworkInfo, true))
                continue;

            // try match filament by vendor filament type
            if (tryMatchFilamentByFilamentType(tray, vendorPresetMap, printerNetworkInfo, false))
                continue;

            // try match filament by vendor generic filament type
            if (tryMatchFilamentByFilamentType(tray, genericPresetMap, printerNetworkInfo, true))
                continue;

            // try match filament by orca generic filament type
            if (tryMatchFilamentByFilamentType(tray, orcaGenericPresetMap, printerNetworkInfo, true))
                continue;

            // the worst case, if all above failed, try match the first filament in the type
            if(boost::to_upper_copy(tray.vendor) != "GENERIC") {
                if(vendorPresetMap.find(tray.filamentType) != vendorPresetMap.end()) {
                    tray.filamentId = vendorPresetMap.at(tray.filamentType)[0].filamentId;
                    tray.settingId = vendorPresetMap.at(tray.filamentType)[0].settingId;
                    tray.filamentPresetName = vendorPresetMap.at(tray.filamentType)[0].filamentName;
                    continue;
                }
            }         
            if(genericPresetMap.find(tray.filamentType) != genericPresetMap.end()) {
                tray.filamentId = genericPresetMap.at(tray.filamentType)[0].filamentId;
                tray.settingId = genericPresetMap.at(tray.filamentType)[0].settingId;
                tray.filamentPresetName = genericPresetMap.at(tray.filamentType)[0].filamentName;
                continue;
            }
            if(orcaGenericPresetMap.find(tray.filamentType) != orcaGenericPresetMap.end()) {
                tray.filamentId = orcaGenericPresetMap.at(tray.filamentType)[0].filamentId;
                tray.settingId = orcaGenericPresetMap.at(tray.filamentType)[0].settingId;
                tray.filamentPresetName = orcaGenericPresetMap.at(tray.filamentType)[0].filamentName;
                continue;
            }
        }
    }
    
    return;
}

// build preset filament map
std::map<std::string, std::vector<PrinterMmsManager::PresetFilamentInfo>> PrinterMmsManager::buildPresetFilamentMap(
    const PresetBundle& bundle, 
    const PrinterNetworkInfo& printerNetworkInfo,
    const std::map<std::string, PrinterPresetInfo>& printerPresetMap,
    const std::vector<double>& currentProjectNozzleDiameters,
    bool isGeneric, bool compatible)
{
    std::map<std::string, std::vector<PresetFilamentInfo>> presetMap;
    
    for (const auto& filament : bundle.filaments) {
        if (!filament.is_system) continue;
        
        auto* filament_type_opt = dynamic_cast<const ConfigOptionStrings*>(filament.config.option("filament_type"));
        if(!filament_type_opt || filament_type_opt->values.empty()) continue;

        if(compatible && !isFilamentCompatible(filament, printerNetworkInfo, printerPresetMap, currentProjectNozzleDiameters)) continue;
        
        // check if is generic filament
        std::string name = boost::to_upper_copy(filament.name);
        bool isGenericFilament = (name.find("GENERIC") != std::string::npos);
        if(isGeneric != isGenericFilament) continue;
        
        PresetFilamentInfo info;
        info.filamentId = filament.filament_id;
        info.settingId = filament.setting_id;
        info.filamentAlias = filament.alias;
        info.filamentName = filament.name;
        info.filamentType = filament_type_opt->values[0];
        
        std::string filamentType = boost::to_upper_copy(info.filamentType);
        presetMap[filamentType].push_back(info);
    }
    
    return presetMap;
}

// check filament compatible
bool PrinterMmsManager::isFilamentCompatible(
    const Preset& filament,
    const PrinterNetworkInfo& printerNetworkInfo,
    const std::map<std::string, PrinterPresetInfo>& printerPresetMap,
    const std::vector<double>& currentProjectNozzleDiameters)
{
    const auto compatiblePrinters = filament.config.option<ConfigOptionStrings>("compatible_printers");
    if(!compatiblePrinters) return false;
    
    for (const std::string& printer_name : compatiblePrinters->values) {
        auto it = printerPresetMap.find(printer_name);
        if(it == printerPresetMap.end() || it->second.printerModel != printerNetworkInfo.printerModel) continue;
        
        if(currentProjectNozzleDiameters.empty()) return true;
        
        for(double currentProjectNozzleDiameter : currentProjectNozzleDiameters) {
            for(double presetNozzle : it->second.nozzleDiameters) {
                if(std::abs(currentProjectNozzleDiameter - presetNozzle) < 0.01) return true;
            }
        }
        return false;
    }
    return false;
}

// try match filament
bool PrinterMmsManager::tryMatchFilament(
    PrinterMmsTray& tray,
    const std::map<std::string, std::vector<PrinterMmsManager::PresetFilamentInfo>>& presetMap,
    const PrinterNetworkInfo& printerNetworkInfo,
    bool isGeneric)
{
    std::string filamentType = boost::to_upper_copy(tray.filamentType);
    auto it = presetMap.find(filamentType);
    if(it == presetMap.end()) return false;
    
    for(const auto& filamentInfo : presetMap.at(filamentType)) {
        if(isNamesMatch(tray, filamentInfo, printerNetworkInfo, isGeneric)) {
            // match success, update tray info
            tray.filamentId = filamentInfo.filamentId;
            tray.settingId = filamentInfo.settingId;
            tray.filamentPresetName = filamentInfo.filamentName;
            return true;
        }
    }
    
    return false;
}

// check filament name match
bool PrinterMmsManager::isNamesMatch(
    const PrinterMmsTray& tray,
    const PrinterMmsManager::PresetFilamentInfo& filamentInfo,
    const PrinterNetworkInfo& printerNetworkInfo,
    bool isGeneric)
{
    std::string presetFilamentAlias = boost::to_upper_copy(filamentInfo.filamentAlias);
    boost::trim(presetFilamentAlias);
    std::string mmsVendor = boost::to_upper_copy(tray.vendor);
    boost::trim(mmsVendor);
    std::string mmsFilamentName = boost::to_upper_copy(tray.filamentName);
    boost::trim(mmsFilamentName);
    std::string presetVendor = boost::to_upper_copy(printerNetworkInfo.vendor);
    boost::trim(presetVendor);
    
    // standardize preset filament alias
    if(isGeneric) {
        // generic filament: remove vendor prefix, add GENERIC prefix
        if(presetFilamentAlias.find(presetVendor) != std::string::npos) {
            boost::erase_all(presetFilamentAlias, presetVendor);
            boost::trim(presetFilamentAlias);
        }
        if(presetFilamentAlias.find("GENERIC") == std::string::npos) {
            presetFilamentAlias = "GENERIC " + presetFilamentAlias;
        }
    } else {
        // vendor filament: ensure contains vendor prefix
        if(presetFilamentAlias.find(presetVendor) == std::string::npos) {
            presetFilamentAlias = presetVendor + " " + presetFilamentAlias;
        }
    }
    
    // standardize mms filament name
    if(isGeneric) {
        // generic filament: remove vendor prefix, add GENERIC prefix
        if(mmsFilamentName.find(mmsVendor) != std::string::npos) {
            boost::erase_all(mmsFilamentName, mmsVendor);
            boost::trim(mmsFilamentName);
        }
        if(mmsFilamentName.find("GENERIC") == std::string::npos) {
            mmsFilamentName = "GENERIC " + mmsFilamentName;
        }
    } else {
        // vendor filament: ensure contains vendor prefix
        if(mmsFilamentName.find(mmsVendor) == std::string::npos) {
            mmsFilamentName = mmsVendor + " " + mmsFilamentName;
        }
    }
    
    
    // replace all dashes with spaces
    std::replace(presetFilamentAlias.begin(), presetFilamentAlias.end(), '-', ' ');
    std::replace(mmsFilamentName.begin(), mmsFilamentName.end(), '-', ' ');

    // remove space and compare
    boost::trim(presetFilamentAlias);
    boost::trim(mmsFilamentName);
    return presetFilamentAlias == mmsFilamentName;
}

// try match filament by filament type

bool PrinterMmsManager::tryMatchFilamentByFilamentType(
    PrinterMmsTray& tray,
    const std::map<std::string, std::vector<PrinterMmsManager::PresetFilamentInfo>>& presetMap,
    const PrinterNetworkInfo& printerNetworkInfo,
    bool isGeneric)
{
    std::string filamentType = boost::to_upper_copy(tray.filamentType);
    if(presetMap.find(filamentType) == presetMap.end()) {    
        // Orca's filament types are not standardized (e.g., PPA type may be configured as PPA-CF). Try using filament name as type for lookup as a fallback.
        filamentType = boost::to_upper_copy(tray.filamentName);
        if(presetMap.find(filamentType) == presetMap.end()) {
            return false;
        }
        
    }

    for(const auto& filamentInfo : presetMap.at(filamentType)) {
        if(isNamesMatch(tray, filamentInfo, printerNetworkInfo, isGeneric)) {
            // match success, update tray info
            tray.filamentId = filamentInfo.filamentId;
            tray.settingId = filamentInfo.settingId;
            tray.filamentPresetName = filamentInfo.filamentName;
            return true;
        }
    }  


    // try match filament by mms filament type and preset filament name
    for(const auto& filamentInfo : presetMap.at(filamentType)) {
        PrinterMmsTray trayCopy = tray;
        trayCopy.filamentName = tray.filamentType;
        if(isNamesMatch(trayCopy, filamentInfo, printerNetworkInfo, isGeneric)) {
            // match success, update tray info
            tray.filamentId = filamentInfo.filamentId;
            tray.settingId = filamentInfo.settingId;
            tray.filamentPresetName = filamentInfo.filamentName;
            return true;
        }
    }

    // try match filament by mms filament type and preset filament type
    for(const auto& filamentInfo : presetMap.at(filamentType)) {
        PrinterMmsTray trayCopy = tray;
        trayCopy.filamentName = tray.filamentType;
        PresetFilamentInfo filamentCopy = filamentInfo;
        filamentCopy.filamentAlias = filamentInfo.filamentType;
        if(isNamesMatch(trayCopy, filamentCopy, printerNetworkInfo, isGeneric)) {
            tray.filamentId = filamentCopy.filamentId;
            tray.settingId = filamentCopy.settingId;
            tray.filamentPresetName = filamentCopy.filamentName;
            return true;
        }
    }


    return false;
}

PrinterNetworkResult<PrinterMmsGroup> PrinterMmsManager::getPrinterMmsInfo(const std::string& printerId)
{
    PrinterNetworkInfo printerNetworkInfo = PrinterManager::getInstance()->getPrinterNetworkInfo(printerId);
    if(printerNetworkInfo.printerId.empty()) {
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, PrinterMmsGroup());
    }
    PrinterNetworkResult<PrinterMmsGroup> mmsGroupResult = PrinterManager::getInstance()->getPrinterMmsInfo(printerId);
    if(!mmsGroupResult.isSuccess()) {
        return mmsGroupResult;
    }
    PrinterMmsGroup mmsGroup = mmsGroupResult.data.value();

    getMmsTrayFilamentId(printerNetworkInfo, mmsGroup);
    
    // match filament id in system preset to mms tray
    // set tray index
    std::vector<std::string> trayIndexList = {"A","B","C","D","E","F","G","H"};
    int mmsIndex = 0;
    for(auto& mms : mmsGroup.mmsList) {
        if(mmsIndex >= trayIndexList.size()) {
            BOOST_LOG_TRIVIAL(error) << "PrinterMmsManager::getPrinterMmsInfo: tray index list is not enough";
            break;
        }
        std::string mmsIndexStr =trayIndexList[mmsIndex];
        int trayIndex = 1;
        mmsIndex++;
        for(auto& tray : mms.trayList) {
            tray.trayName = mmsIndexStr + std::to_string(trayIndex);
            trayIndex++;        
        }
    }
    return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::SUCCESS, mmsGroup);
}

bool PrinterMmsManager::checkTrayIsReady(const PrinterMmsTray& tray) {
    if(tray.status != TRAY_STATUS_LOADED && tray.status != TRAY_STATUS_PRELOADED) {
        return false;
    }
    if(tray.filamentType.empty() || tray.filamentName.empty() || tray.filamentColor.empty()) {
        return false;
    }
    return true;
};

void PrinterMmsManager::getFilamentMmsMapping(const PrinterNetworkInfo& printerNetworkInfo, std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping, const PrinterMmsGroup& mmsGroup)
{
    // Load mapping from JSON file
    nlohmann::json mappingJson = loadFilamentMmsMappingFromFile();
    
    for (auto& printFilament : printFilamentMmsMapping) {
        StandardColor standardColor = getStandardColor(printFilament.filamentColor);
        std::string filamentStandardColor = standardColor.colorHex;
        std::string StandardColorName = standardColor.colorName;
        if(StandardColorName.empty() || filamentStandardColor.empty()) {
            wxLogError("PrinterMmsManager::getFilamentMmsMapping: standard color name or filament standard color is empty, filamentType: %s, filamentAlias: %s, filamentColor: %s", printFilament.filamentType.c_str(), printFilament.filamentAlias.c_str(), printFilament.filamentColor.c_str());
            continue;
        }
        std::string mmsMappingFilamentType = "";
        std::string mmsMappingFilamentName = "";
        std::string mmsMappingFilamentColor = "";
        bool isMapped = false;
        
        // Check if mapping exists in JSON
        if (mappingJson.contains(printFilament.filamentType) &&
            mappingJson[printFilament.filamentType].contains(printFilament.filamentAlias) &&
            mappingJson[printFilament.filamentType][printFilament.filamentAlias].contains(filamentStandardColor)) {
            
            auto mapping = mappingJson[printFilament.filamentType][printFilament.filamentAlias][filamentStandardColor];
            mmsMappingFilamentType = mapping.value("mappedFilamentType", "");
            mmsMappingFilamentName = mapping.value("mappedFilamentName", "");
            mmsMappingFilamentColor = mapping.value("mappedFilamentColor", "");
        }
        if (!mmsMappingFilamentType.empty() && !mmsMappingFilamentName.empty() && !mmsMappingFilamentColor.empty()) {
            for (auto& mms : mmsGroup.mmsList) {
                for (auto& tray : mms.trayList) {
                    if(!checkTrayIsReady(tray)) {
                        continue;
                    }
                    if (boost::to_upper_copy(tray.filamentType) == boost::to_upper_copy(mmsMappingFilamentType) &&
                        boost::to_upper_copy(tray.filamentName) == boost::to_upper_copy(mmsMappingFilamentName) &&
                        boost::to_upper_copy(tray.filamentColor) == boost::to_upper_copy(mmsMappingFilamentColor)) {
                        printFilament.mappedMmsFilament.trayId           = tray.trayId;
                        printFilament.mappedMmsFilament.mmsId            = tray.mmsId;
                        printFilament.mappedMmsFilament.trayName         = tray.trayName;
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
        }

        if (isMapped) {
            continue;
        }
        // not mapped or mapped filament not exist
        std::map<std::string, std::vector<PrinterMmsManager::PresetFilamentInfo>> filamentPresetMap;
        PrinterMmsManager::PresetFilamentInfo filamentInfo;
        filamentInfo.filamentId = printFilament.filamentId;
        filamentInfo.settingId = printFilament.settingId;
        filamentInfo.filamentAlias = printFilament.filamentAlias;
        filamentInfo.filamentName = printFilament.filamentName;
        filamentInfo.filamentType = printFilament.filamentType;
        filamentPresetMap[boost::to_upper_copy(printFilament.filamentType)].push_back(filamentInfo);
        PrinterMmsTray mappedTray;
        for (auto& mms : mmsGroup.mmsList) {
            for (auto& tray : mms.trayList) {
                if(!checkTrayIsReady(tray)) {
                    continue;
                }
                // check if the filament color is the same as the standard color
                if(boost::to_upper_copy(getStandardColor(tray.filamentColor).colorHex) != boost::to_upper_copy(filamentStandardColor)){
                    continue;
                }
                mappedTray = tray;
                // try match filament by filament name if not match, try match filament by generic preset
                if(tryMatchFilament(mappedTray, filamentPresetMap, printerNetworkInfo, false) || tryMatchFilament(mappedTray, filamentPresetMap, printerNetworkInfo, true)) {
                    isMapped = true;
                    break;
                }
            }
        }
        if(!isMapped) {
            // if not match by filament name, try match filament by filament type
            for (auto& mms : mmsGroup.mmsList) {
                for (auto& tray : mms.trayList) {
                    if(!checkTrayIsReady(tray)) {
                        continue;
                    }
                    if(boost::to_upper_copy(getStandardColor(tray.filamentColor).colorHex) != boost::to_upper_copy(filamentStandardColor)){
                        continue;
                    }
                    mappedTray = tray;
                    // try match filament by filament type if not match, try match filament by generic preset
                    if(tryMatchFilamentByFilamentType(mappedTray, filamentPresetMap, printerNetworkInfo, false) || tryMatchFilamentByFilamentType(mappedTray, filamentPresetMap, printerNetworkInfo, true)) {
                        isMapped = true;
                        break;
                    }
                }
            }
        }
        if(isMapped) {
            printFilament.mappedMmsFilament.trayName         = mappedTray.trayName;
            printFilament.mappedMmsFilament.trayId           = mappedTray.trayId;
            printFilament.mappedMmsFilament.mmsId            = mappedTray.mmsId;
            printFilament.mappedMmsFilament.filamentName     = mappedTray.filamentName;
            printFilament.mappedMmsFilament.filamentColor    = mappedTray.filamentColor;
            printFilament.mappedMmsFilament.filamentType     = mappedTray.filamentType;
            printFilament.mappedMmsFilament.filamentDiameter = mappedTray.filamentDiameter;
            printFilament.mappedMmsFilament.minNozzleTemp    = mappedTray.minNozzleTemp;
            printFilament.mappedMmsFilament.maxNozzleTemp    = mappedTray.maxNozzleTemp;
            printFilament.mappedMmsFilament.minBedTemp       = mappedTray.minBedTemp;
            printFilament.mappedMmsFilament.maxBedTemp       = mappedTray.maxBedTemp;
            printFilament.mappedMmsFilament.status           = mappedTray.status;
        }
    }
}

void PrinterMmsManager::saveFilamentMmsMapping(std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping)
{
    // Load existing mappings first
    nlohmann::json mappingJson = loadFilamentMmsMappingFromFile();
    
    for (auto& printFilament : printFilamentMmsMapping) {
        if (printFilament.mappedMmsFilament.trayName.empty() || 
            printFilament.mappedMmsFilament.trayId.empty() || 
            printFilament.mappedMmsFilament.filamentName.empty() ||
            printFilament.mappedMmsFilament.filamentColor.empty()) {
            continue;
        }
        
        StandardColor mappedStandardColor = getStandardColor(printFilament.mappedMmsFilament.filamentColor);
        std::string mappedFilamentStandardColor = mappedStandardColor.colorHex;
        std::string mappedStandardColorName = mappedStandardColor.colorName;
        
        if(mappedStandardColorName.empty() || mappedFilamentStandardColor.empty()) {
            wxLogError("PrinterMmsManager::saveFilamentMmsMapping: color name or filament standard color is empty, filamentType: %s, filamentAlias: %s, filamentColor: %s", printFilament.filamentType.c_str(), printFilament.filamentAlias.c_str(), printFilament.filamentColor.c_str());
            continue;
        }
        StandardColor standardColor = getStandardColor(printFilament.filamentColor);
        std::string filamentStandardColor = standardColor.colorHex;
        std::string filamentStandardColorName = standardColor.colorName;
        
        if(filamentStandardColorName.empty() || filamentStandardColor.empty()) {
            wxLogError("PrinterMmsManager::saveFilamentMmsMapping: color name or filament standard color is empty, filamentType: %s, filamentAlias: %s, filamentColor: %s", printFilament.filamentType.c_str(), printFilament.filamentAlias.c_str(), printFilament.filamentColor.c_str());
            continue;
        }
        // Create three-level structure: filamentType -> filamentAlias -> filamentStandardColor
        if (!mappingJson.contains(printFilament.filamentType)) {
            mappingJson[printFilament.filamentType] = nlohmann::json::object();
        }
        if (!mappingJson[printFilament.filamentType].contains(printFilament.filamentAlias)) {
            mappingJson[printFilament.filamentType][printFilament.filamentAlias] = nlohmann::json::object();
        }

        // save mapping information, mapped filament info, print filament color converted to standard color 
        // when model multi-color editing, the same standard color will have many color values, unified converted to standard color
        // mapped filament color is the mms filament actual color, but also store the mms filament color corresponding to the standard color
        mappingJson[printFilament.filamentType][printFilament.filamentAlias][filamentStandardColor] = {
            {"mappedFilamentType", printFilament.mappedMmsFilament.filamentType},
            {"mappedFilamentName", printFilament.mappedMmsFilament.filamentName},
            {"mappedFilamentColor", printFilament.mappedMmsFilament.filamentColor},
            {"mappedFilamentStandardColor", mappedFilamentStandardColor},
            {"mappedFilamentStandardColorName", mappedStandardColorName},
            {"filamentStandardColor", filamentStandardColor},
            {"filamentStandardColorName", filamentStandardColorName}
        };
    }
    
    // Save merged mappings to JSON file
    saveFilamentMmsMappingToFile(mappingJson);
}

nlohmann::json PrinterMmsManager::loadFilamentMmsMappingFromFile()
{
    std::lock_guard<std::mutex> lock(mFilamentMmsMappingMutex);
    try {
        
        std::string filePath = (boost::filesystem::path(Slic3r::data_dir()) / "user" / "filament_mms_mapping.json").string();
        
        if (boost::filesystem::exists(filePath)) {
            boost::nowide::ifstream file(filePath);
            if (file.is_open()) {
                nlohmann::json jsonData;
                file >> jsonData;
                file.close();
                return jsonData;
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load filament MMS mapping file: %s", e.what());
    }
    
    return nlohmann::json::object();
}

void PrinterMmsManager::saveFilamentMmsMappingToFile(const nlohmann::json& mappingJson)
{
    std::lock_guard<std::mutex> lock(mFilamentMmsMappingMutex);
    try {
        std::string filePath = (boost::filesystem::path(Slic3r::data_dir()) / "user" / "filament_mms_mapping.json").string();
        
        // Ensure directory exists
        boost::filesystem::path dir = boost::filesystem::path(filePath).parent_path();
        if (!boost::filesystem::exists(dir)) {
            boost::filesystem::create_directories(dir);
        }
        
        boost::nowide::ofstream file(filePath);
        if (file.is_open()) {
            file << mappingJson.dump(4); // Pretty print with 4 spaces indentation
            file.close();
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to save filament MMS mapping file: %s", e.what());
    }
}

void PrinterMmsManager::removeFilamentMmsMapping(const std::string& filamentType, const std::string& filamentAlias, const std::string& filamentColor)
{
    std::lock_guard<std::mutex> lock(mFilamentMmsMappingMutex);
    try {
        // Load existing mappings
        nlohmann::json mappingJson = loadFilamentMmsMappingFromFile();
        
        std::string filamentStandardColor = getStandardColor(filamentColor).colorHex;
        
        // Remove specific mapping if it exists
        if (mappingJson.contains(filamentType) &&
            mappingJson[filamentType].contains(filamentAlias) &&
            mappingJson[filamentType][filamentAlias].contains(filamentStandardColor)) {
            
            mappingJson[filamentType][filamentAlias].erase(filamentStandardColor);
            
            // Clean up empty objects
            if (mappingJson[filamentType][filamentAlias].empty()) {
                mappingJson[filamentType].erase(filamentAlias);
            }
            if (mappingJson[filamentType].empty()) {
                mappingJson.erase(filamentType);
            }
            
            // Save updated mappings
            saveFilamentMmsMappingToFile(mappingJson);
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to remove filament MMS mapping: %s", e.what());
    }
}

} // namespace Slic3r
