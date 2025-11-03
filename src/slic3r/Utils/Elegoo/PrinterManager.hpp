#pragma once
#include <map>
#include "slic3r/Utils/Elegoo/PrinterNetwork.hpp"
#include "slic3r/Utils/Singleton.hpp"
#include "slic3r/Utils/Elegoo/PrinterNetwork.hpp"

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
    PrinterNetworkResult<PrinterPrintFileResponse> getFileList(const std::string& printerId, int pageNumber, int pageSize);
    PrinterNetworkResult<PrinterPrintTaskResponse> getPrintTaskList(const std::string& printerId, int pageNumber, int pageSize);
    PrinterNetworkResult<bool> deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds);
    PrinterNetworkResult<bool> sendRtmMessage(const std::string& printerId, const std::string& message);
    PrinterNetworkResult<PrinterPrintFileResponse> getFileDetail(const std::string& printerId, const std::string& fileName);

    static std::map<std::string, std::map<std::string, DynamicPrintConfig>> getVendorPrinterModelConfig();
    static std::string imageFileToBase64DataURI(const std::string& image_path);

    void init();
    void close();

private:
    PrinterManager();
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

    std::mutex mPrinterNetworkMutex;
    std::map<std::string, std::shared_ptr<IPrinterNetwork>> mPrinterNetworkConnections;
    bool addPrinterNetwork(const std::shared_ptr<IPrinterNetwork>& network);
    bool deletePrinterNetwork(const std::string& printerId);
    std::shared_ptr<IPrinterNetwork> getPrinterNetwork(const std::string& printerId);

        
    // sync old preset printers to network
    void syncOldPresetPrinters();
    
    // Validate and complete printer info with system preset
    void validateAndCompletePrinterInfo(PrinterNetworkInfo& printerInfo);

    // refresh online printers from user network
    std::mutex mOnlinePrintersMutex;
    std::chrono::steady_clock::time_point mLastRefreshOnlinePrintersTime;
    void refreshOnlinePrinters(bool force = false);
    std::string generatePrinterId();

    
    std::atomic<bool> mIsInitialized;
    std::mutex mInitializedMutex;

    // thread to monitor printer connections
    std::thread mConnectionThread;
    std::chrono::steady_clock::time_point mLastConnectionLoopTime;
    void monitorPrinterConnections();
    
    // Check and handle WAN network error (like token expiration)
    template<typename T>
    void checkUserAuthStatus(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkResult<T>& result, 
                             const UserNetworkInfo& requestUserInfo);
};
} // namespace Slic3r::GUI 
