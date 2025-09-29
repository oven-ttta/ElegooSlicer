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
#define CONFIG_KEY_SWITCH_TO_DEVICE_TAB "printsend_switch_to_device_tab"

#define CONFIG_KEY_MMS_FILAMENT_MAPPING "mms_filament_mapping"

class PrinterManager : public Singleton<PrinterManager>
{
    friend class Singleton<PrinterManager>;
public:
    ~PrinterManager();
    PrinterManager(const PrinterManager&) = delete;
    PrinterManager& operator=(const PrinterManager&) = delete;

    PrinterNetworkInfo getPrinterNetworkInfo(const std::string& printerId);
    std::vector<PrinterNetworkInfo> getPrinterList();
    PrinterNetworkInfo getSelectedPrinter(const std::string &printerModel, const std::string &printerId);

    PrinterNetworkResult<bool> upload(PrinterNetworkParams& params);
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinter();
    PrinterNetworkResult<bool> addPrinter(PrinterNetworkInfo& printerNetworkInfo);
    PrinterNetworkResult<bool> updatePrinterName(const std::string& printerId, const std::string& name);
    PrinterNetworkResult<bool> updatePrinterHost(const std::string& printerId, const std::string& host);
    PrinterNetworkResult<bool> deletePrinter(const std::string& printerId);
    PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo(const std::string& printerId);


    PrinterNetworkResult<std::vector<PrinterPrintFile>> getFileList(const std::string& printerId);
    PrinterNetworkResult<std::vector<PrinterPrintTask>> getPrintTaskList(const std::string& printerId);
    PrinterNetworkResult<bool> deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds);

    // WAN
    PrinterNetworkResult<std::string> getRtcToken();
    PrinterNetworkResult<bool> sendRtmMessage(const std::string& message);
    PrinterNetworkResult<bool> onRtcTokenChanged(const std::string& printerId);
    PrinterNetworkResult<bool> onRtmMessage(const std::string& printerId, const std::string& message);
    PrinterNetworkResult<bool> onConnectionStatus(const std::string& printerId, const std::string& status);
    PrinterNetworkResult<bool> onPrinterEventRaw(const std::string& printerId, const std::string& event);


    static std::map<std::string, std::map<std::string, DynamicPrintConfig>> getVendorPrinterModelConfig();
    static std::string imageFileToBase64DataURI(const std::string& image_path);

    void init();
    void close();
    
    // sync old preset printers to network
    void syncOldPresetPrinters();

    PrinterNetworkResult<bool> loginWAN(const NetworkUserInfo& userInfo);
private:

    class PrinterLock
    {
    public:
        PrinterLock(const std::string& printerId);
        ~PrinterLock();
        
    private:
        std::mutex* mPrinterMutex;
        static std::map<std::string, std::mutex> sPrinterMutexes;
        static std::mutex sMutex;       
    };

private:
    PrinterManager();

    std::mutex mConnectionsMutex;
    std::map<std::string, std::shared_ptr<IPrinterNetwork>> mNetworkConnections;
    bool addPrinterNetwork(const std::shared_ptr<IPrinterNetwork>& network);
    bool deletePrinterNetwork(const std::string& printerId);
    std::shared_ptr<IPrinterNetwork> getPrinterNetwork(const std::string& printerId);

    // thread to monitor printer connections
    std::atomic<bool> mIsRunning;
    std::thread mConnectionThread;
    void monitorPrinterConnections();

    NetworkUserInfo mNetworkUserInfo;
    std::shared_ptr<IPrinterNetwork> mNetworkWAN;
};
} // namespace Slic3r::GUI 
