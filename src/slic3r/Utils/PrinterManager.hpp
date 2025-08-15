#pragma once
#include <map>
#include "slic3r/Utils/PrinterNetwork.hpp"
#include "slic3r/Utils/Singleton.hpp"

namespace Slic3r { 

// Multi-device management
// Support multiple device types, but need to adapt network communication, implement PrinterNetwork class
// Network layer supports 1-to-1, many-to-1

#define CONFIG_KEY_PATH    "printhost_path"
#define CONFIG_KEY_GROUP   "printhost_group"
#define CONFIG_KEY_STORAGE "printhost_storage"

#define CONFIG_KEY_UPLOADANDPRINT      "printsend_upload_and_print"
#define CONFIG_KEY_TIMELAPSE           "printsend_timelapse"
#define CONFIG_KEY_HEATEDBEDLEVELING   "printsend_heated_bed_leveling"
#define CONFIG_KEY_BEDTYPE             "printsend_bed_type"
#define CONFIG_KEY_AUTO_REFILL         "printsend_auto_refill"
#define CONFIG_KEY_SELECTED_PRINTER_ID "printsend_selected_printer_id"

#define CONFIG_KEY_MMS_FILAMENT_MAPPING "mms_filament_mapping"

class PrinterManager : public Singleton<PrinterManager>
{
    friend class Singleton<PrinterManager>;
public:
    ~PrinterManager();
    PrinterManager(const PrinterManager&) = delete;
    PrinterManager& operator=(const PrinterManager&) = delete;

    std::vector<PrinterNetworkInfo> getPrinterList();
    bool upload(PrinterNetworkParams& params);
    std::vector<PrinterNetworkInfo> discoverPrinter();
    std::string addPrinter(PrinterNetworkInfo& printerNetworkInfo);
    bool updatePrinterName(const std::string& printerId, const std::string& name);
    bool updatePrinterHost(const std::string& printerId, const std::string& host);
    bool deletePrinter(const std::string& printerId);
    PrinterMmsGroup getPrinterMmsInfo(const std::string& printerId);
    PrinterNetworkInfo getPrinterNetworkInfo(const std::string& printerId);

    static PrinterNetworkInfo convertJsonToPrinterNetworkInfo(const nlohmann::json& json);
    static nlohmann::json convertPrinterNetworkInfoToJson(const PrinterNetworkInfo& printerNetworkInfo);
    static std::map<std::string, std::map<std::string, DynamicPrintConfig>> getVendorPrinterModelConfig();
    static std::string imageFileToBase64DataURI(const std::string& image_path);

    void close();

private:
    PrinterManager();

    void savePrinterList();

    void updatePrinterConnectStatus(const std::string& printerId, const PrinterConnectStatus& status);
    void updatePrinterStatus(const std::string& printerId, const PrinterStatus& status);
    void updatePrinterPrintTask(const std::string& printerId, const PrinterPrintTask& task);
    void updatePrinterAttributes(const std::string& printerId, const PrinterNetworkInfo& printerInfo);

    std::map<std::string, PrinterNetworkInfo> mPrinterList;
    std::mutex mPrinterListMutex;

};
} // namespace Slic3r::GUI 
