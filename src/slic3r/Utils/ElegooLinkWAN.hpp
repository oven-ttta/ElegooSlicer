#ifndef ELEGOO_LINK_WAN_HPP
#define ELEGOO_LINK_WAN_HPP

#include <string>
#include <memory>
#include <functional>
#include <elegoolink/type.h>
#include "nlohmann/json.hpp"
namespace elink {
#ifdef ERROR_INVALID_LIBRARY
#undef ERROR_INVALID_LIBRARY
#endif

// Result type for loader operations
enum class LoaderResult {
    SUCCESS                    = 0,
    ERROR_LIBRARY_NOT_FOUND    = 1,
    ERROR_SYMBOL_NOT_FOUND     = 2,
    ERROR_VERSION_INCOMPATIBLE = 3,
    ERROR_ALREADY_LOADED       = 4,
    ERROR_NOT_LOADED           = 5,
    ERROR_PLATFORM_UNSUPPORTED = 6,
    ERROR_INVALID_LIBRARY      = 7
};

struct NetworkConfig
{
    NetworkConfig() : logLevel(1), logEnableConsole(true), logEnableFile(false), logMaxFileSize(10 * 1024 * 1024), logMaxFiles(5) {}
    // Logging configuration
    int         logLevel;         // Log level 0 - TRACE, 1 - DEBUG, 2 - INFO, 3 - WARN, 4 - ERROR, 5 - CRITICAL, 6 - OFF
    bool        logEnableConsole; // Enable log output to console
    bool        logEnableFile;    // Enable log output to file
    std::string logFileName;      // Log file name
    size_t      logMaxFileSize;   // Maximum log file size (10MB)
    size_t      logMaxFiles;      // Maximum number of log files

    std::string baseMqttUrl; // Base MQTT URL, e.g. "wss://rtm.elegoo.com:443", if empty, will use default
    std::string baseApiUrl;  // Base API URL, e.g. "https://api.elegoo.com", if empty, will use default
    std::string userAgent;   // User-Agent string, if empty, will use default
};

struct RtcTokenData
{
    int64_t     userId;             // 用户ID
    std::string rtcToken;           // RTC令牌
    int64_t     rtcTokenExpireTime; // RTC令牌过期时间（秒时间戳）
};

using GetRtcTokenResult = BizResult<RtcTokenData>;

struct SetRegionParams : public BaseParams
{
    std::string region; // Region identifier, e.g., "us", "eu", "asia"
};

struct GetWebUrlData
{
    std::string loginUrl;
    std::string modelLibraryUrl;
};

using GetWebUrlResult = BizResult<GetWebUrlData>;

struct UserInfo
{
    int64_t     id;
    std::string email;
    std::string name;
    std::string avatar;
};

struct HttpCredential
{
    int64_t     userId;
    std::string accessToken;
    std::string refreshToken;
    int64_t     accessTokenExpireTime;
    int64_t     refreshTokenExpireTime;
};

using GetUserInfoResult = BizResult<UserInfo>;

struct BindPrinterParams
{
    std::string name;         // Printer name, e.g., "Elegoo Neptune 3", user customizable
    std::string model;        // Printer model
    std::string serialNumber; // Printer serial number
    std::string authMode;     // Authentication mode, "pinCode"
    std::string pinCode;      // PIN code
};

struct BindPrinterData
{
    bool        bindResult;
    PrinterInfo printerInfo; // Printer information
};

using BindPrinterResult = BizResult<BindPrinterData>;

struct UnbindPrinterParams : public PrinterBaseParams
{};

using UnbindPrinterResult = VoidResult;

struct SendRtmMessageParams : public PrinterBaseParams
{
    std::string message; // 消息内容
};

struct RtmMessageData
{
    std::string printerId; // Printer ID
    std::string message;   // 响应内容
};

struct PrinterEventRawData
{
    std::string printerId; // Printer ID
    std::string rawData;   // Raw data
};

struct PrintTaskDetail
{
    std::string taskId;        // Task ID
    std::string thumbnail;     // Thumbnail URL
    std::string taskName;      // Task name
    int64_t     beginTime = 0; // Start time (timestamp/seconds)
    int64_t     endTime   = 0; // End time (timestamp/seconds)
    /** Task status
        0: Other status
        1: Completed
        2: Exception status
        3: Stopped
     */
    int taskStatus = 0; // Task status
    // /**
    //  * Time-lapse video status
    //  * 0: No time-lapse video file
    //  * 1: Time-lapse video file exists
    //  * 2: Time-lapse video file deleted
    //  * 3: Time-lapse video generating
    //  * 4: Time-lapse video generation failed
    //  */
    // int timeLapseVideoStatus = 0;   // Time-lapse video status
    // std::string timeLapseVideoUrl;  // Time-lapse video URL
    // int timeLapseVideoSize = 0;     // Time-lapse video size (FDM)
    // int timeLapseVideoDuration = 0; // Time-lapse video duration (seconds) (FDM)
};

/**
 * Get print task list request parameters
 */
struct PrintTaskListParams : public PrinterBaseParams
{
    int pageNumber = 1;  // Page number, starting from 1
    int pageSize   = 50; // Page size, maximum value is 100
};

/**
 * Print task list data
 */
struct PrintTaskListData
{
    // Print task ID list
    std::vector<PrintTaskDetail> taskList;       // Print task detail list
    int                          totalTasks = 0; // Total number of print tasks
};

using PrintTaskListResult = BizResult<PrintTaskListData>;

/**
 * Batch delete historical print tasks request parameters
 */
struct DeletePrintTasksParams : public PrinterBaseParams
{
    std::vector<std::string> taskIds; // Print task ID list
};

using DeletePrintTasksResult = VoidResult;
/**
 * File detail
 */
struct FileDetail
{
    std::string fileName;                       // File name
    int64_t     printTime   = 0;                // Print time (seconds)
    int         layer       = 0;                // Total layers
    double      layerHeight = 0;                // Layer height (millimeters)
    std::string thumbnail;                      // Thumbnail URL
    int64_t     size                    = 0;    // File size (bytes)
    int64_t     createTime              = 0;    // File creation time (timestamp/seconds)
    double      totalFilamentUsed       = 0.0f; // Estimated total filament consumption weight (grams)
    double      totalFilamentUsedLength = 0.0f; // Estimated total filament consumption length (millimeters)
    int         totalPrintTimes         = 0;    // Total print times
    int64_t     lastPrintTime           = 0;    // Last print time (timestamp/seconds)
};
/**
 * Get file list request parameters
 */
struct GetFileListParams : public PrinterBaseParams
{
    int pageNumber = 1;  // Page number, starting from 1
    int pageSize   = 50; // Page size, maximum value is 100
};
/**
 * File list data
 */
struct GetFileListData
{
    /**
     * File list
     */
    std::vector<FileDetail> fileList;       // File list
    int                     totalFiles = 0; // Total number of files
};

using GetFileListResult = BizResult<GetFileListData>;

class RtmMessageEvent : public BaseEvent
{
public:
    RtmMessageData message; // Connection status data
};

/*
 * RTC Token Changed Event
 */
class RtcTokenEvent : public BaseEvent
{
public:
    RtcTokenData token; // RTC Token data
};

/**
 * Logged in elsewhere event
 */
class LoggedInElsewhereEvent : public BaseEvent
{};

class PrinterEventRawEvent : public BaseEvent
{
public:
    PrinterEventRawData rawData;
};

class ElegooLinkWAN
{
public:
    using FileUploadProgressCallback = std::function<bool(const FileUploadProgressData& progress)>;
    using LogCallback                = std::function<void(int level, const std::string& message)>;

private:
    ElegooLinkWAN()  = default;

public:
    ~ElegooLinkWAN()                 = default;
    /**
     * Get singleton instance
     * @return Reference to ElegooLink singleton instance
     */
    static ElegooLinkWAN& getInstance();
    LoaderResult loadLibrary(const std::string& library_path);
    LoaderResult unloadLibrary();
    VoidResult   initialize(const NetworkConfig& config);
    void         cleanup();
    bool         isInitialized() const;
    void         setLogCallback(LogCallback callback);
    std::string  version() const;

    VoidResult setRegion(const SetRegionParams& params);

    GetWebUrlResult getWebUrl();

    GetUserInfoResult         getUserInfo();
    VoidResult                setHttpCredential(const HttpCredential& credential);
    BizResult<HttpCredential> getHttpCredential() const;
    VoidResult                clearHttpCredential();
    GetRtcTokenResult         getRtcToken() const;
    GetPrinterListResult      getPrinters();
    VoidResult                sendRtmMessage(const SendRtmMessageParams& params);

    ConnectPrinterResult connectPrinter(const ConnectPrinterParams& params);

    VoidResult disconnectPrinter(const std::string& printerId);

    BindPrinterResult bindPrinter(const BindPrinterParams& params);

    VoidResult unbindPrinter(const UnbindPrinterParams& params);

    GetFileListResult getFileList(const GetFileListParams& params);

    PrintTaskListResult getPrintTaskList(const PrintTaskListParams& params);

    DeletePrintTasksResult deletePrintTasks(const DeletePrintTasksParams& params);

    StartPrintResult startPrint(const StartPrintParams& params);

    GetCanvasStatusResult getCanvasStatus(const GetCanvasStatusParams& params);

    VoidResult setAutoRefill(const SetAutoRefillParams& params);

    PrinterStatusResult getPrinterStatus(const PrinterStatusParams& params);

    PrinterAttributesResult getPrinterAttributes(const PrinterAttributesParams& params);

    /**
     * Refresh printer attributes, The result will be notified through events
     * @param params Printer attributes parameters
     * @return Operation result
     */
    VoidResult refreshPrinterAttributes(const PrinterAttributesParams& params);
    /**
     * Refresh printer status, The result will be notified through events
     * @param params Printer status parameters
     * @return Operation result
     */
    VoidResult refreshPrinterStatus(const PrinterStatusParams& params);

    BizResult<std::string> getPrinterStatusRaw(const PrinterStatusParams& params);

    FileUploadCompletionResult uploadFile(const FileUploadParams& params, FileUploadProgressCallback progressCallback = nullptr);
    /**
     * Event subscription ID type
     */
    using EventSubscriptionId = EventBus::EventId;
    /**
     * General strongly-typed event subscription method
     * @param handler Event handler function
     * @return Subscription ID, can be used to unsubscribe
     */
    template<typename EventType> EventSubscriptionId subscribeEvent(std::function<void(const std::shared_ptr<EventType>&)> handler)
    {
        return eventBus_.subscribe<EventType>(handler);
    }

    /**
     * Unsubscribe event
     * @param id Subscription ID
     * @return Whether unsubscribed successfully
     */
    template<typename EventType> bool unsubscribeEvent(EventSubscriptionId id) { return eventBus_.unsubscribe<EventType>(id); }



private:
    class ElegooLinkWANImpl;
    std::unique_ptr<ElegooLinkWANImpl> impl_;
    EventBus                           eventBus_;
};

} // namespace elink
#endif // ELEGOO_LINK_WAN_HPP