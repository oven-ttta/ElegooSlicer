#ifndef slic3r_PrinterMmsManager_hpp_
#define slic3r_PrinterMmsManager_hpp_

#include <map>
#include "slic3r/Utils/Singleton.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include <nlohmann/json.hpp>

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

    static nlohmann::json convertPrinterMmsTrayToJson(const PrinterMmsTray& tray);
    static PrinterMmsTray convertJsonToPrinterMmsTray(const nlohmann::json& json);
    static nlohmann::json convertPrinterMmsToJson(const PrinterMms& mms);
    static PrinterMms convertJsonToPrinterMms(const nlohmann::json& json);
    static nlohmann::json convertPrinterMmsGroupToJson(const PrinterMmsGroup& mmsGroup);
    static PrinterMmsGroup convertJsonToPrinterMmsGroup(const nlohmann::json& json);
    static nlohmann::json convertPrintFilamentMmsMappingToJson(const PrintFilamentMmsMapping& printFilamentMmsMapping);
    static PrintFilamentMmsMapping convertJsonToPrintFilamentMmsMapping(const nlohmann::json& json);

private:
    PrinterMmsManager();

};
} // namespace Slic3r::GUI 
#endif 
