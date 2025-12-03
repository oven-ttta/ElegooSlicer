#ifndef slic3r_PrinterMmsManager_hpp_
#define slic3r_PrinterMmsManager_hpp_

#include <map>
#include <mutex>
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

    PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const std::string& printerId);
  
    void getFilamentMmsMapping(const PrinterNetworkInfo& printerNetworkInfo, std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping, const PrinterMmsGroup& mmsGroup);
    void saveFilamentMmsMapping(std::vector<PrintFilamentMmsMapping>& printFilamentMmsMapping);
    void removeFilamentMmsMapping(const std::string& filamentType, const std::string& filamentAlias, const std::string& filamentColor);

private:
    PrinterMmsManager();

    void getMmsTrayFilamentId(const PrinterNetworkInfo& printerNetworkInfo, PrinterMmsGroup& mmsGroup);
    
    // JSON file operations
    nlohmann::json loadFilamentMmsMappingFromFile();
    void saveFilamentMmsMappingToFile(const nlohmann::json& mappingJson);

    struct PresetFilamentInfo
    {
        std::string filamentId;
        std::string settingId;
        std::string filamentAlias;
        std::string filamentName;
        std::string filamentType;
    };

    struct PrinterPresetInfo
    {
        std::string printerModel;
        std::vector<double> nozzleDiameters;
    };

    std::map<std::string, std::vector<PresetFilamentInfo>> buildPresetFilamentMap(
        const PresetBundle& bundle, 
        const PrinterNetworkInfo& printerNetworkInfo,
        const std::map<std::string, PrinterPresetInfo>& printerPresetMap,
        const std::vector<double>& currentProjectNozzleDiameters,
        bool isGeneric, bool compatible);
    
    bool isFilamentCompatible(const Preset& filament,
                             const PrinterNetworkInfo& printerNetworkInfo,
                             const std::map<std::string, PrinterPresetInfo>& printerPresetMap,
                             const std::vector<double>& currentProjectNozzleDiameters);
    
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
    bool checkTrayIsReady(const PrinterMmsTray& tray);


    std::mutex mFilamentMmsMappingMutex;
};
} // namespace Slic3r::GUI 
#endif 
