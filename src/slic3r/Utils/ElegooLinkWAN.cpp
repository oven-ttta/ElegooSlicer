#include "ElegooLinkWAN.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#define GET_PROC_ADDRESS GetProcAddress
#define LOAD_LIBRARY LoadLibraryA
#define FREE_LIBRARY FreeLibrary
#define LIBRARY_EXTENSION ".dll"

#ifdef ERROR_INVALID_LIBRARY
#undef ERROR_INVALID_LIBRARY
#endif

#elif defined(__APPLE__)
#include <dlfcn.h>
#define GET_PROC_ADDRESS dlsym
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define FREE_LIBRARY dlclose
#define LIBRARY_EXTENSION ".dylib"
#else // Linux and other Unix-like systems
#include <dlfcn.h>
#define GET_PROC_ADDRESS dlsym
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define FREE_LIBRARY dlclose
#define LIBRARY_EXTENSION ".so"
#endif

// form elegoolink
namespace elink {

inline void to_json(nlohmann::json& j, const elink::BaseParams&) { j = nlohmann::json::object(); }
inline void from_json(const nlohmann::json& j, std::monostate& nlohmann_json_t) {}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrinterBaseParams, printerId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrinterInfo,
                                                printerId,
                                                printerType,
                                                brand,
                                                manufacturer,
                                                name,
                                                model,
                                                firmwareVersion,
                                                serialNumber,
                                                mainboardId,
                                                host,
                                                webUrl,
                                                authMode,
                                                extraInfo)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(StorageComponent, name, removable)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FanComponent, name, controllable, minSpeed, maxSpeed, supportsRpmReading)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    TemperatureComponent, name, controllable, supportsTemperatureReading, minTemperature, maxTemperature)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LightComponent, name, type, minBrightness, maxBrightness)

// Nested structs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CameraCapabilities, supportsCamera, supportsTimeLapse)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SystemCapabilities, canSetPrinterName, canGetDiskInfo, supportsMultiFilament)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PrintCapabilities, supportsAutoBedLeveling, supportsTimeLapse, supportsHeatedBedSwitching, supportsFilamentMapping)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrinterCapabilities,
                                                storageComponents,
                                                fanComponents,
                                                temperatureComponents,
                                                lightComponents,
                                                cameraCapabilities,
                                                systemCapabilities,
                                                printCapabilities)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrinterAttributes,
                                                printerId,
                                                printerType,
                                                brand,
                                                manufacturer,
                                                name,
                                                model,
                                                firmwareVersion,
                                                serialNumber,
                                                mainboardId,
                                                host,
                                                webUrl,
                                                authMode,
                                                extraInfo,
                                                capabilities)
// Status structs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrinterStatus, state, subState, exceptionCodes, supportProgress, progress)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TemperatureStatus, current, target, highest, lowest)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FanStatus, speed, rpm)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrintAxesStatus, position)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PrintStatus, taskId, fileName, totalTime, currentTime, estimatedTime, totalLayer, currentLayer, progress, printSpeedMode)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LightStatus, connected, brightness, color)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(StorageStatus, connected)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SlotMapItem, t, trayId, canvasId)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    TrayInfo, trayId, brand, filamentType, filamentName, filamentCode, filamentColor, minNozzleTemp, maxNozzleTemp, status)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CanvasInfo, name, model, canvasId, connected, trays)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CanvasStatus, activeCanvasId, activeTrayId, autoRefill, canvases)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExternalDeviceStatus, usbConnected, sdCardConnected, cameraConnected, canvasConnected)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrinterStatusData,
                                                printerId,
                                                printerStatus,
                                                printStatus,
                                                temperatureStatus,
                                                fanStatus,
                                                printAxesStatus,
                                                lightStatus,
                                                storageStatus,
                                                canvasStatus,
                                                externalDeviceStatus)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    StartPrintParams, printerId, storageLocation, fileName, autoBedLeveling, heatedBedType, enableTimeLapse, slotMap)
// File transfer structs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FileUploadParams, printerId, storageLocation, localFilePath, fileName, overwriteExisting)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FileUploadProgressData, taskId, fileName, totalBytes, uploadedBytes, percentage)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CancelFileUploadParams, printerId, taskId)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FileUploadCompletionData, fileName)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConnectionStatusData, printerId, status)
// SetAutoRefillParams
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SetAutoRefillParams, printerId, enable)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GetPrinterListData, printers)

} // namespace

namespace elink {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    NetworkConfig, logLevel, logEnableConsole, logEnableFile, logFileName, logMaxFileSize, logMaxFiles, baseMqttUrl, baseApiUrl, userAgent)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SetRegionParams, region)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GetWebUrlData, loginUrl, modelLibraryUrl)
// GetUserInfoParams
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UserInfo, id, email, name, avatar)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    HttpCredential, userId, accessToken, refreshToken, accessTokenExpireTime, refreshTokenExpireTime)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtcTokenData, userId, rtcToken, rtcTokenExpireTime)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrintTaskDetail, taskId, thumbnail, taskName, beginTime, endTime, taskStatus)

// Task-related structs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrintTaskListParams, printerId, pageNumber, pageSize)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrintTaskListData, taskList, totalTasks)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DeletePrintTasksParams, printerId, taskIds)

// File-related structs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GetFileListParams, printerId, pageNumber, pageSize)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GetFileListData, fileList, totalFiles)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FileDetail,
                                                fileName,
                                                printTime,
                                                layer,
                                                layerHeight,
                                                thumbnail,
                                                size,
                                                createTime,
                                                totalFilamentUsed,
                                                totalFilamentUsedLength,
                                                totalPrintTimes,
                                                lastPrintTime)
// RtmMessageData
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtmMessageData, printerId, message)

// SendRtmMessageParams
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SendRtmMessageParams, printerId, message)

// BindPrinterParams
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BindPrinterParams, name, authMode, model, serialNumber, pinCode)

// BindPrinterData
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BindPrinterData, bindResult, printerInfo)

// UnbindPrinterParams
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UnbindPrinterParams, printerId)

} // namespace elink

namespace elink {
// Forward declarations for version info
struct ElegooNetworkVersion
{
    int         major;
    int         minor;
    int         patch;
    std::string full_version;

    ElegooNetworkVersion(int maj = 0, int min = 0, int p = 0, const std::string& full = "")
        : major(maj), minor(min), patch(p), full_version(full)
    {}
};
struct BizEvent
{
    std::string    method;
    nlohmann::json data;
};
typedef void (*ElegooNetWorkLogCallback)(int level, const char* message, void* userdata);
typedef void (*ElegooNetWorkEventCallback)(const char* eventJson, void* userdata);
// Function pointer types for the elegoo_network API
typedef const char* (*elegoo_network_get_version_func)();
typedef int (*elegoo_network_initialize_func)(const char* configJson);
typedef void (*elegoo_network_cleanup_func)();
typedef int (*elegoo_network_is_initialized_func)();
typedef int (*elegoo_network_request_func)(const char*                requestJson,
                                           char**                     responseJson,
                                           ElegooNetWorkEventCallback eventCallback,
                                           void*                      userdata);
typedef void (*elegoo_network_free_response_func)(char* responseJson);
typedef int (*elegoo_network_set_event_callback_func)(ElegooNetWorkEventCallback callback, void* userdata);
typedef int (*elegoo_network_set_log_callback_func)(ElegooNetWorkLogCallback callback, void* userdata);
typedef const char* (*elegoo_network_get_error_string_func)(int errorCode);

/**
 * Cross-platform dynamic library loader for ElegooNetwork
 * Supports Windows (.dll), Linux (.so), and macOS (.dylib)
 *
 * Features:
 * - Version compatibility checking (major version must match)
 * - Thread-safe loading/unloading
 * - Automatic symbol resolution
 * - Error reporting
 */
class ElegooNetworkLoader
{
public:
    /**
     * Constructor
     * @param required_major_version Required major version for compatibility (e.g., 2 for 2.x.x)
     */
    explicit ElegooNetworkLoader(int required_major_version = 1);

    /**
     * Destructor - automatically unloads library if loaded
     */
    ~ElegooNetworkLoader();

    // Disable copy constructor and assignment
    ElegooNetworkLoader(const ElegooNetworkLoader&)            = delete;
    ElegooNetworkLoader& operator=(const ElegooNetworkLoader&) = delete;

    /**
     * Load the ElegooNetwork dynamic library
     * @param library_path Path to the library file
     * @return LoaderResult indicating success or failure reason
     */
    LoaderResult loadLibrary(const std::string& library_path);

    /**
     * Unload the currently loaded library
     * @return LoaderResult indicating success or failure reason
     */
    LoaderResult unloadLibrary();

    /**
     * Check if library is currently loaded
     * @return true if loaded, false otherwise
     */
    bool isLoaded() const;

    /**
     * Get version information of the loaded library
     * @return ElegooNetworkVersion struct with version info, or empty version if not loaded
     */
    ElegooNetworkVersion getLibraryVersion() const;

    /**
     * Check if the loaded library version is compatible
     * @return true if compatible, false otherwise
     */
    bool isVersionCompatible() const;

    /**
     * Get the required major version
     * @return Required major version number
     */
    int getRequiredMajorVersion() const { return requiredMajorVersion_; }

    /**
     * Get last error message
     * @return Error message string
     */
    std::string getLastError() const { return lastError_; }

    // API function accessors - these return nullptr if library not loaded or symbols not found

    /**
     * Get the elegoo_network_get_version function pointer
     */
    elegoo_network_get_version_func getVersionFunc() const { return get_version_; }

    /**
     * Get the elegoo_network_initialize function pointer
     */
    elegoo_network_initialize_func getInitializeFunc() const { return initialize_; }

    /**
     * Get the elegoo_network_cleanup function pointer
     */
    elegoo_network_cleanup_func getCleanupFunc() const { return cleanup_; }

    /**
     * Get the elegoo_network_is_initialized function pointer
     */
    elegoo_network_is_initialized_func getIsInitializedFunc() const { return is_initialized_; }

    /**
     * Get the elegoo_network_request function pointer
     */
    elegoo_network_request_func getRequestFunc() const { return request_; }

    /**
     * Get the elegoo_network_free_response function pointer
     */
    elegoo_network_free_response_func getFreeResponseFunc() const { return free_response_; }

    /**
     * Get the elegoo_network_set_event_callback function pointer
     */
    elegoo_network_set_event_callback_func getSetEventCallbackFunc() const { return set_event_callback_; }

    /**
     * Get the elegoo_network_set_log_callback function pointer
     */
    elegoo_network_set_log_callback_func getSetLogCallbackFunc() const { return set_log_callback_; }

    /**
     * Get the elegoo_network_get_error_string function pointer
     */
    elegoo_network_get_error_string_func getErrorStringFunc() const { return get_error_string_; }

    /**
     * Convenience function to get the platform-specific library filename
     * @param base_name Base name without extension (e.g., "ElegooNetwork")
     * @return Platform-specific filename (e.g., "ElegooNetwork.dll" on Windows)
     */
    static std::string getPlatformLibraryName(const std::string& base_name);

    /**
     * Get string representation of LoaderResult
     * @param result The LoaderResult enum value
     * @return Human-readable string description
     */
    static std::string resultToString(LoaderResult result);

private:
    /**
     * Platform-specific library handle type
     */
#ifdef _WIN32
    using LibraryHandle = void*; // HMODULE
#else
    using LibraryHandle = void*; // Handle from dlopen
#endif

    /**
     * Load all required symbols from the library
     * @return LoaderResult indicating success or failure
     */
    LoaderResult loadSymbols();

    /**
     * Parse version string into ElegooNetworkVersion struct
     * @param version_str Version string (e.g., "2.1.0")
     * @return Parsed version information
     */
    ElegooNetworkVersion parseVersion(const std::string& version_str) const;

    /**
     * Set last error message
     * @param message Error message
     */
    void setError(const std::string& message) { lastError_ = message; }

    // Member variables
    int                  requiredMajorVersion_;
    LibraryHandle        libraryHandle_;
    ElegooNetworkVersion loadedVersion_;
    mutable std::string  lastError_;

    // Function pointers
    elegoo_network_get_version_func        get_version_;
    elegoo_network_initialize_func         initialize_;
    elegoo_network_cleanup_func            cleanup_;
    elegoo_network_is_initialized_func     is_initialized_;
    elegoo_network_request_func            request_;
    elegoo_network_free_response_func      free_response_;
    elegoo_network_set_event_callback_func set_event_callback_;
    elegoo_network_set_log_callback_func   set_log_callback_;
    elegoo_network_get_error_string_func   get_error_string_;
};

ElegooNetworkLoader::ElegooNetworkLoader(int required_major_version)
    : requiredMajorVersion_(required_major_version)
    , libraryHandle_(nullptr)
    , loadedVersion_()
    , lastError_()
    , get_version_(nullptr)
    , initialize_(nullptr)
    , cleanup_(nullptr)
    , is_initialized_(nullptr)
    , request_(nullptr)
    , free_response_(nullptr)
    , set_event_callback_(nullptr)
    , set_log_callback_(nullptr)
    , get_error_string_(nullptr)
{}

ElegooNetworkLoader::~ElegooNetworkLoader() { unloadLibrary(); }

LoaderResult ElegooNetworkLoader::loadLibrary(const std::string& library_path)
{
    if (libraryHandle_ != nullptr) {
        setError("Library already loaded");
        return LoaderResult::ERROR_ALREADY_LOADED;
    }

    // Load the library
#ifdef _WIN32
    libraryHandle_ = LoadLibraryExA(library_path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
#else
    libraryHandle_ = LOAD_LIBRARY(library_path.c_str());
#endif

    if (libraryHandle_ == nullptr) {
        std::string error_msg = "Failed to load library: " + library_path;
#ifdef _WIN32
        DWORD error_code = GetLastError();
        error_msg += " (Error code: " + std::to_string(error_code) + ")";
#else
        const char* dl_error = dlerror();
        if (dl_error != nullptr) {
            error_msg += " (" + std::string(dl_error) + ")";
        }
#endif
        setError(error_msg);
        return LoaderResult::ERROR_LIBRARY_NOT_FOUND;
    }

    // Load all required symbols
    LoaderResult symbol_result = loadSymbols();
    if (symbol_result != LoaderResult::SUCCESS) {
        unloadLibrary();
        return symbol_result;
    }

    // Get and check version compatibility
    if (get_version_ != nullptr) {
        const char* version_buffer = get_version_();
        if (version_buffer != nullptr) { // Assuming 0 means success
            loadedVersion_ = parseVersion(std::string(version_buffer));
            if (!isVersionCompatible()) {
                std::stringstream ss;
                ss << "Version incompatible. Required major version: " << requiredMajorVersion_
                   << ", loaded version: " << loadedVersion_.full_version;
                setError(ss.str());
                unloadLibrary();
                return LoaderResult::ERROR_VERSION_INCOMPATIBLE;
            }
        } else {
            setError("Failed to get library version");
            unloadLibrary();
            return LoaderResult::ERROR_INVALID_LIBRARY;
        }
    }

    return LoaderResult::SUCCESS;
}

LoaderResult ElegooNetworkLoader::unloadLibrary()
{
    if (libraryHandle_ == nullptr) {
        return LoaderResult::ERROR_NOT_LOADED;
    }

    // Call cleanup if available and if library is initialized
    if (cleanup_ != nullptr && is_initialized_ != nullptr && is_initialized_()) {
        cleanup_();
    }

    // Clear function pointers
    get_version_        = nullptr;
    initialize_         = nullptr;
    cleanup_            = nullptr;
    is_initialized_     = nullptr;
    request_            = nullptr;
    free_response_      = nullptr;
    set_event_callback_ = nullptr;
    set_log_callback_   = nullptr;
    get_error_string_   = nullptr;

    // Unload library
    bool success = false;
#ifdef _WIN32
    success = (FREE_LIBRARY(static_cast<HMODULE>(libraryHandle_)) != 0);
#else
    success = (FREE_LIBRARY(libraryHandle_) == 0);
#endif

    libraryHandle_ = nullptr;
    loadedVersion_ = ElegooNetworkVersion();

    if (!success) {
        setError("Failed to unload library");
        return LoaderResult::ERROR_PLATFORM_UNSUPPORTED;
    }

    return LoaderResult::SUCCESS;
}

bool ElegooNetworkLoader::isLoaded() const { return libraryHandle_ != nullptr; }

ElegooNetworkVersion ElegooNetworkLoader::getLibraryVersion() const { return loadedVersion_; }

bool ElegooNetworkLoader::isVersionCompatible() const { return loadedVersion_.major == requiredMajorVersion_; }

LoaderResult ElegooNetworkLoader::loadSymbols()
{
    // Define a lambda to load a symbol and check for errors
    auto load_symbol = [this](const char* symbol_name, void** func_ptr) -> bool {
        *func_ptr = GET_PROC_ADDRESS((HMODULE) libraryHandle_, symbol_name);
        if (*func_ptr == nullptr) {
            std::string error_msg = "Failed to find symbol: " + std::string(symbol_name);
#ifndef _WIN32
            const char* dl_error = dlerror();
            if (dl_error != nullptr) {
                error_msg += " (" + std::string(dl_error) + ")";
            }
#endif
            setError(error_msg);
            return false;
        }
        return true;
    };

    // Load all required symbols
    if (!load_symbol("elegoo_network_get_version", reinterpret_cast<void**>(&get_version_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_initialize", reinterpret_cast<void**>(&initialize_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_cleanup", reinterpret_cast<void**>(&cleanup_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_is_initialized", reinterpret_cast<void**>(&is_initialized_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_request", reinterpret_cast<void**>(&request_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_free_response", reinterpret_cast<void**>(&free_response_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_set_event_callback", reinterpret_cast<void**>(&set_event_callback_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_set_log_callback", reinterpret_cast<void**>(&set_log_callback_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    if (!load_symbol("elegoo_network_get_error_string", reinterpret_cast<void**>(&get_error_string_))) {
        return LoaderResult::ERROR_SYMBOL_NOT_FOUND;
    }

    return LoaderResult::SUCCESS;
}

ElegooNetworkVersion ElegooNetworkLoader::parseVersion(const std::string& version_str) const
{
    ElegooNetworkVersion version(0, 0, 0, version_str);

    // Use regex to parse version string like "1.2.3" or "1.2.3-beta"
    std::regex version_regex(R"((\d+)\.(\d+)\.(\d+)(?:-.*)?)");
    ;
    std::smatch matches;

    if (std::regex_match(version_str, matches, version_regex) && matches.size() >= 4) {
        try {
            version.major = std::stoi(matches[1].str());
            version.minor = std::stoi(matches[2].str());
            version.patch = std::stoi(matches[3].str());
        } catch (const std::exception& e) {
            // If parsing fails, keep default values (0.0.0)
        }
    }

    return version;
}

std::string ElegooNetworkLoader::getPlatformLibraryName(const std::string& base_name)
{
#ifdef _WIN32
    return base_name + ".dll";
#elif defined(__APPLE__)
    return "lib" + base_name + ".dylib";
#else
    return "lib" + base_name + ".so";
#endif
}

std::string ElegooNetworkLoader::resultToString(LoaderResult result)
{
    switch (result) {
    case LoaderResult::SUCCESS: return "Success";
    case LoaderResult::ERROR_LIBRARY_NOT_FOUND: return "Library not found";
    case LoaderResult::ERROR_SYMBOL_NOT_FOUND: return "Required symbol not found in library";
    case LoaderResult::ERROR_VERSION_INCOMPATIBLE: return "Library version incompatible";
    case LoaderResult::ERROR_ALREADY_LOADED: return "Library already loaded";
    case LoaderResult::ERROR_NOT_LOADED: return "No library currently loaded";
    case LoaderResult::ERROR_PLATFORM_UNSUPPORTED: return "Platform not supported or system error";
    case LoaderResult::ERROR_INVALID_LIBRARY: return "Invalid library or failed to get version";
    default: return "Unknown error";
    }
}

class ElegooLinkWAN::ElegooLinkWANImpl
{
public:
    ElegooLinkWANImpl();
    ~ElegooLinkWANImpl() {}

    std::unique_ptr<ElegooNetworkLoader> m_loader;
    LogCallback                          logCallback_;
    std::string                          makeRawRequest(const std::string&                   method,
                                                        const std::string&                   paramsJson,
                                                        std::function<void(const BizEvent&)> eventCallback = nullptr) const
    {
        if (!m_loader || !m_loader->isLoaded()) {
            throw std::runtime_error("Library not loaded");
        }
        auto request_func = m_loader->getRequestFunc();
        auto free_func    = m_loader->getFreeResponseFunc();
        if (!request_func || !free_func) {
            throw std::runtime_error("Failed to get request function pointer");
        }
        nlohmann::json request_json;
        request_json["method"] = method;
        if (!paramsJson.empty() && paramsJson != "{}") {
            try {
                request_json["params"] = nlohmann::json::parse(paramsJson);
            } catch (...) {
                request_json["params"] = nlohmann::json::object();
            }
        } else {
            request_json["params"] = nlohmann::json::object();
        }

        struct RequestContext
        {
            std::function<void(const BizEvent&)> eventCallback;
        };

        RequestContext context{eventCallback};

        std::string      requestStr     = request_json.dump();
        char*            responseStr    = nullptr;
        ELINK_ERROR_CODE request_result = (ELINK_ERROR_CODE) (request_func(
            requestStr.c_str(), &responseStr,
            [](const char* eventJson, void* userdata) {
                try {
                    RequestContext* context = static_cast<RequestContext*>(userdata);
                    if (context == nullptr) {
                        return;
                    }
                    nlohmann::json event = nlohmann::json::parse(eventJson, nullptr, false);
                    if (event.contains("method")) {
                        std::string method = event["method"].get<std::string>();
                        if (event.contains("data") && event["data"].is_object()) {
                            nlohmann::json data = event["data"];
                            BizEvent       biz_event{method, data};
                            if (context->eventCallback) {
                                context->eventCallback(biz_event);
                            }
                        }
                    }
                } catch (...) {}
            },
            &context));
        if (responseStr) {
            std::string resp_str_copy(responseStr);
            free_func(responseStr);
            return resp_str_copy;
        }
        throw std::runtime_error("Request failed with code: " + std::to_string((int) request_result));
    }

    // Helper template to extract template argument from BizResult<T>
    template<typename T> struct template_arg
    {
        using type = std::monostate; // Default for non-template types
    };

    template<template<typename> class Container, typename T> struct template_arg<Container<T>>
    {
        using type = T;
    };

    template<typename TResult> TResult parseResponse(const std::string& responseJson) const
    {
        // Extract the type of the data field from template parameter
        using type0 = typename template_arg<TResult>::type;

        try {
            nlohmann::json response_json = nlohmann::json::parse(responseJson, nullptr, false);
            TResult        result;
            if (response_json.contains("code") && response_json["code"].is_number_integer()) {
                result.code = (ELINK_ERROR_CODE) response_json["code"].get<int>();
            }
            if (response_json.contains("message") && response_json["message"].is_string()) {
                result.message = response_json["message"].get<std::string>();
            }
            if (response_json.contains("data") && response_json["data"].is_object()) {
                if constexpr (std::is_same_v<type0, std::monostate>) {
                    return result;
                }

                type0 data;
                from_json(response_json["data"], data);
                result.data = data;
            }
            return result;
        } catch (const std::exception& e) {
            return TResult::Error(ELINK_ERROR_CODE::UNKNOWN_ERROR, "Failed to parse response JSON: " + std::string(e.what()));
        }
    }

    template<typename TParams, typename TResult>
    TResult makeRequest(const std::string& method, const TParams& params, std::function<void(const BizEvent&)> eventCallback = nullptr) const
    {
        try {
            nlohmann::json j;
            to_json(j, params);
            std::string paramsJson = j.dump();
            std::string response   = makeRawRequest(method, paramsJson, eventCallback);
            return parseResponse<TResult>(response);
        } catch (const std::exception& e) {
            return TResult::Error(ELINK_ERROR_CODE::UNKNOWN_ERROR, e.what());
        }
    }

    template<typename TResult>
    TResult makeRequest(const std::string& method, std::function<void(const BizEvent&)> eventCallback = nullptr) const
    {
        try {
            std::string paramsJson = "{}";
            std::string response   = makeRawRequest(method, paramsJson, eventCallback);
            return parseResponse<TResult>(response);
        } catch (const std::exception& e) {
            return TResult::Error(ELINK_ERROR_CODE::UNKNOWN_ERROR, e.what());
        }
    }
};

ElegooLinkWAN::ElegooLinkWANImpl::ElegooLinkWANImpl() : m_loader(std::make_unique<ElegooNetworkLoader>(1)) {}
ElegooLinkWAN& ElegooLinkWAN::getInstance() {
    static ElegooLinkWAN instance;
    return instance;
}
// ElegooLinkWAN接口调用Pimpl
LoaderResult ElegooLinkWAN::loadLibrary(const std::string& library_path)
{
    if (!impl_)
        impl_ = std::make_unique<ElegooLinkWANImpl>();
    std::string lib_path = library_path;
    if (lib_path.empty()) {
        lib_path = ElegooNetworkLoader::getPlatformLibraryName("ElegooNetwork");
    }
    return impl_->m_loader->loadLibrary(lib_path);
}
LoaderResult ElegooLinkWAN::unloadLibrary()
{
    if (impl_ && impl_->m_loader) {
        return impl_->m_loader->unloadLibrary();
    }
    return LoaderResult::ERROR_NOT_LOADED;
}

VoidResult ElegooLinkWAN::initialize(const NetworkConfig& config)
{
    if (!impl_)
        impl_ = std::make_unique<ElegooLinkWANImpl>();
    if (!impl_->m_loader || !impl_->m_loader->isLoaded()) {
        return VoidResult::Error(ELINK_ERROR_CODE::NOT_INITIALIZED, "Library not loaded");
    }
    auto initialize_func     = impl_->m_loader->getInitializeFunc();
    auto is_initialized_func = impl_->m_loader->getIsInitializedFunc();
    if (!initialize_func || !is_initialized_func) {
        return VoidResult::Error(ELINK_ERROR_CODE::NOT_INITIALIZED, "Failed to get required function pointers");
    }
    const char*      configStr   = nlohmann::json(config).dump().c_str();
    ELINK_ERROR_CODE init_result = (ELINK_ERROR_CODE) (initialize_func(configStr));
    if (init_result != ELINK_ERROR_CODE::SUCCESS) {
        auto        error_string_func = impl_->m_loader->getErrorStringFunc();
        std::string error_msg         = "SDK initialization failed (code: " + std::to_string((int) init_result) + ")";
        if (error_string_func) {
            const char* error_str = error_string_func((int) init_result);
            if (error_str) {
                error_msg += ", " + std::string(error_str);
            }
        }
        return VoidResult::Error(init_result, error_msg);
    }

    auto set_event_callback_func = impl_->m_loader->getSetEventCallbackFunc();
    if (set_event_callback_func) {
        set_event_callback_func(
            [](const char* event_json, void* userdata) {
                try {
                    ElegooLinkWAN* self = static_cast<ElegooLinkWAN*>(userdata);
                    if (self == nullptr) {
                        return;
                    }
                    nlohmann::json event = nlohmann::json::parse(event_json, nullptr, false);
                    if (event.contains("method")) {
                        std::string method = event["method"].get<std::string>();
                        if (event.contains("data") && event["data"].is_object()) {
                            nlohmann::json data = event["data"];
                            if (method == "on_printer_status") {
                                PrinterStatusData status;
                                from_json(data, status);
                                auto event    = std::make_shared<PrinterStatusEvent>();
                                event->status = status;
                                self->eventBus_.publish(event);
                            } else if (method == "on_printer_attributes") {
                                PrinterAttributesData attributes;
                                from_json(data, attributes);
                                auto event        = std::make_shared<PrinterAttributesEvent>();
                                event->attributes = attributes;
                                self->eventBus_.publish(event);
                            } else if (method == "on_connection_status") {
                                ConnectionStatusData status;
                                from_json(data, status);
                                auto event              = std::make_shared<PrinterConnectionEvent>();
                                event->connectionStatus = status;
                                self->eventBus_.publish(event);
                            } else if (method == "on_rtm_message") {
                                RtmMessageData message;
                                from_json(data, message);
                                auto event     = std::make_shared<RtmMessageEvent>();
                                event->message = message;
                                self->eventBus_.publish(event);
                            } else if (method == "on_rtc_token_changed") {
                                RtcTokenData tokenData;
                                from_json(data, tokenData);
                                auto event   = std::make_shared<RtcTokenEvent>();
                                event->token = tokenData;
                                self->eventBus_.publish(event);
                            } else if (method == "on_logged_in_elsewhere") {
                                auto event      = std::make_shared<LoggedInElsewhereEvent>();
                                self->eventBus_.publish(event);
                            }
                        }
                    }
                } catch (...) {}
            },
            this);
    }

    return VoidResult::Success();
}

void ElegooLinkWAN::cleanup()
{
    if (impl_ && impl_->m_loader && impl_->m_loader->isLoaded()) {

        auto set_log_callback_func = impl_->m_loader->getSetLogCallbackFunc();
        if (set_log_callback_func) {
            set_log_callback_func(nullptr, nullptr);
        }

        auto set_event_callback_func = impl_->m_loader->getSetEventCallbackFunc();
        if (set_event_callback_func) {
            set_event_callback_func(nullptr, nullptr);
        }
        
        auto cleanup_func = impl_->m_loader->getCleanupFunc();
        if (cleanup_func) {
            cleanup_func();
        }
    }
}

bool ElegooLinkWAN::isInitialized() const
{
    if (!impl_ || !impl_->m_loader || !impl_->m_loader->isLoaded()) {
        return false;
    }
    auto is_initialized_func = impl_->m_loader->getIsInitializedFunc();
    if (!is_initialized_func) {
        return false;
    }
    return is_initialized_func() != 0;
}

void ElegooLinkWAN::setLogCallback(LogCallback callback)
{
    if (!impl_ || !impl_->m_loader || !impl_->m_loader->isLoaded()) {
        return;
    }
    impl_->logCallback_        = callback;
    auto set_log_callback_func = impl_->m_loader->getSetLogCallbackFunc();
    if (set_log_callback_func) {
        set_log_callback_func(
            [](int level, const char* message, void* userdata) {
                try {
                    ElegooLinkWAN* self = static_cast<ElegooLinkWAN*>(userdata);
                    if (self == nullptr || !self->impl_ || !self->impl_->logCallback_) {
                        return;
                    }
                    self->impl_->logCallback_(level, message);
                } catch (...) {}
            },
            this);
    }
}

std::string ElegooLinkWAN::version() const
{
    if (!impl_ || !impl_->m_loader || !impl_->m_loader->isLoaded()) {
        return "";
    }
    auto get_version_func = impl_->m_loader->getVersionFunc();
    if (!get_version_func) {
        return "";
    }
    return get_version_func();
}

VoidResult ElegooLinkWAN::setRegion(const SetRegionParams& params)
{
    return impl_->makeRequest<SetRegionParams, VoidResult>("set_region", params);
}

GetWebUrlResult ElegooLinkWAN::getWebUrl() { return impl_->makeRequest<GetWebUrlResult>("get_web_url"); }

GetUserInfoResult ElegooLinkWAN::getUserInfo() { return impl_->makeRequest<GetUserInfoResult>("get_user_info"); }

VoidResult ElegooLinkWAN::setHttpCredential(const HttpCredential& credential)
{
    return impl_->makeRequest<HttpCredential, VoidResult>("set_http_credential", credential);
}

BizResult<HttpCredential> ElegooLinkWAN::getHttpCredential() const
{
    return impl_->makeRequest<BizResult<HttpCredential>>("get_http_credential");
}

VoidResult ElegooLinkWAN::clearHttpCredential() { return impl_->makeRequest<VoidResult>("clear_http_credential"); }

GetRtcTokenResult ElegooLinkWAN::getRtcToken() const { return impl_->makeRequest<GetRtcTokenResult>("get_rtc_token"); }

GetPrinterListResult ElegooLinkWAN::getPrinters() { return impl_->makeRequest<GetPrinterListResult>("get_printers"); }

VoidResult ElegooLinkWAN::sendRtmMessage(const SendRtmMessageParams& params)
{
    return impl_->makeRequest<SendRtmMessageParams, VoidResult>("send_rtm_message", params);
}
ConnectPrinterResult ElegooLinkWAN::connectPrinter(const ConnectPrinterParams& params)
{
    auto statusResult = getPrinterStatus({params.serialNumber});
    if (statusResult.isSuccess()) {
        auto attributesResult = getPrinterAttributes({params.serialNumber});
        if (attributesResult.isSuccess()) {
            if (attributesResult.data.has_value()) {
                auto attributes = attributesResult.value();

                PrinterInfo printerInfo;
                printerInfo.printerId       = attributes.printerId;
                printerInfo.printerType     = attributes.printerType;
                printerInfo.brand           = attributes.brand;
                printerInfo.manufacturer    = attributes.manufacturer;
                printerInfo.name            = attributes.name;
                printerInfo.model           = attributes.model;
                printerInfo.firmwareVersion = attributes.firmwareVersion;
                printerInfo.serialNumber    = attributes.serialNumber;
                printerInfo.mainboardId     = attributes.mainboardId;
                printerInfo.host            = attributes.host;
                printerInfo.webUrl          = attributes.webUrl;
                printerInfo.authMode        = attributes.authMode;
                printerInfo.extraInfo       = attributes.extraInfo;

                ConnectPrinterData data;
                data.printerInfo = printerInfo;
                return ConnectPrinterResult::Ok(data);
            } else {
                return ConnectPrinterResult::Error(ELINK_ERROR_CODE::PRINTER_INVALID_RESPONSE, "Printer attributes data is missing");
            }
        } else {
            return ConnectPrinterResult::Error(attributesResult.code, attributesResult.message);
        }
    }
    return ConnectPrinterResult::Error(statusResult.code, statusResult.message);
}

VoidResult           ElegooLinkWAN::disconnectPrinter(const std::string& printerId){ 
    return VoidResult::Success();
}
BindPrinterResult ElegooLinkWAN::bindPrinter(const BindPrinterParams& params)
{
    return impl_->makeRequest<BindPrinterParams, BindPrinterResult>("bind_printer", params);
}

VoidResult ElegooLinkWAN::unbindPrinter(const UnbindPrinterParams& params)
{
    return impl_->makeRequest<UnbindPrinterParams, VoidResult>("unbind_printer", params);
}

GetFileListResult ElegooLinkWAN::getFileList(const GetFileListParams& params)
{
    return impl_->makeRequest<GetFileListParams, GetFileListResult>("get_file_list", params);
}

PrintTaskListResult ElegooLinkWAN::getPrintTaskList(const PrintTaskListParams& params)
{
    return impl_->makeRequest<PrintTaskListParams, PrintTaskListResult>("get_print_task_list", params);
}

DeletePrintTasksResult ElegooLinkWAN::deletePrintTasks(const DeletePrintTasksParams& params)
{
    return impl_->makeRequest<DeletePrintTasksParams, DeletePrintTasksResult>("delete_print_tasks", params);
}

StartPrintResult ElegooLinkWAN::startPrint(const StartPrintParams& params)
{
    return impl_->makeRequest<StartPrintParams, StartPrintResult>("start_print", params);
}

GetCanvasStatusResult ElegooLinkWAN::getCanvasStatus(const GetCanvasStatusParams& params)
{
    return impl_->makeRequest<GetCanvasStatusParams, GetCanvasStatusResult>("get_canvas_status", params);
}

VoidResult ElegooLinkWAN::setAutoRefill(const SetAutoRefillParams& params)
{
    return impl_->makeRequest<SetAutoRefillParams, VoidResult>("set_auto_refill", params);
}

PrinterStatusResult ElegooLinkWAN::getPrinterStatus(const PrinterStatusParams& params)
{
    return impl_->makeRequest<PrinterStatusParams, PrinterStatusResult>("get_printer_status", params);
}

PrinterAttributesResult ElegooLinkWAN::getPrinterAttributes(const PrinterAttributesParams& params)
{
    return impl_->makeRequest<PrinterAttributesParams, PrinterAttributesResult>("get_printer_attributes", params);
}

VoidResult ElegooLinkWAN::refreshPrinterAttributes(const PrinterAttributesParams& params)
{
    return impl_->makeRequest<PrinterAttributesParams, VoidResult>("refresh_printer_attributes", params);
}

VoidResult ElegooLinkWAN::refreshPrinterStatus(const PrinterStatusParams& params)
{
    return impl_->makeRequest<PrinterStatusParams, VoidResult>("refresh_printer_status", params);
}

BizResult<std::string> ElegooLinkWAN::getPrinterStatusRaw(const PrinterStatusParams& params)
{
    return impl_->makeRequest<PrinterStatusParams, BizResult<std::string>>("get_printer_status_raw", params);
}

FileUploadCompletionResult ElegooLinkWAN::uploadFile(const FileUploadParams& params, FileUploadProgressCallback progressCallback)
{
    return FileUploadCompletionResult::Error(ELINK_ERROR_CODE::OPERATION_NOT_IMPLEMENTED, "Not implemented");
}
} // namespace elink