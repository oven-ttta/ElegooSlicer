#ifndef slic3r_PrinterMmsManager_hpp_
#define slic3r_PrinterMmsManager_hpp_

#include <map>
#include "slic3r/Utils/Singleton.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include <nlohmann/json.hpp>
#include "libslic3r/PresetBundle.hpp"
namespace Slic3r { 

class PrinterMmsManager : public Singleton<PrinterMmsManager>
{
    friend class Singleton<PrinterMmsManager>;
public:
    ~PrinterMmsManager();
    PrinterMmsManager(const PrinterMmsManager&) = delete;
    PrinterMmsManager& operator=(const PrinterMmsManager&) = delete;

    PrinterMmsGroup getPrinterMmsInfo(const std::string& printerId);
  
    void getFilamentMmsMapping(std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping, const PrinterMmsGroup& mmsGroup);
    void saveFilamentMmsMapping(std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping);



private:
    PrinterMmsManager();

    void getMmsTrayFilamentId(const PrinterNetworkInfo& printerNetworkInfo, PrinterMmsGroup& mmsGroup);

    struct PresetFilamentInfo
    {
        std::string filamentId;
        std::string settingId;
        std::string filamentAlias;
        std::string filamentName;
        std::string filamentType;
    };

    std::map<std::string, std::vector<PresetFilamentInfo>> buildPresetFilamentMap(
        const PresetBundle& bundle, 
        const PrinterNetworkInfo& printerNetworkInfo,
        const std::map<std::string, std::string>& printerNameModelMap,
        bool isGeneric, bool compatible);
    
    bool isFilamentCompatible(const Preset& filament,
                             const PrinterNetworkInfo& printerNetworkInfo,
                             const std::map<std::string, std::string>& printerNameModelMap);
    
    bool tryMatchFilament(PrinterMmsTray& tray,
                         const std::map<std::string, std::vector<PresetFilamentInfo>>& presetMap,
                         const PrinterNetworkInfo& printerNetworkInfo,
                         bool isGeneric);
    
    bool isNamesMatch(const PrinterMmsTray& tray,
                     const PresetFilamentInfo& filamentInfo,
                     const PrinterNetworkInfo& printerNetworkInfo,
                     bool isGeneric);

    bool tryMatchFilamentByFilamentType(PrinterMmsTray& tray,
                                        const std::map<std::string, std::vector<PresetFilamentInfo>>& presetMap,
                                        const PrinterNetworkInfo& printerNetworkInfo,
                                        bool isGeneric);
};
} // namespace Slic3r::GUI 
#endif 
