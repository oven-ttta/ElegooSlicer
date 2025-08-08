#ifndef slic3r_PrinterNetworkResult_hpp_
#define slic3r_PrinterNetworkResult_hpp_

#include <string>
#include <optional>
#include <string_view>

namespace Slic3r {

/**
 * Printer network error codes for the slicer software
 * Aligned with ElegooError design
 */
enum class PrinterNetworkErrorCode
{
    // Success status
    SUCCESS = 0,                    // Operation successful

    // General errors (1-99)
    UNKNOWN_ERROR = 1,              // Unknown error
    INTERNAL_ERROR = 2,             // Internal error
    INVALID_PARAMETER = 3,          // Invalid parameter (value error, type error, etc.)
    INVALID_FORMAT = 4,             // Format error (JSON structure, data format, etc.)
    TIMEOUT = 5,                    // Operation timeout (including connection, file transfer, etc.)
    CANCELLED = 6,                  // Operation cancelled
    PERMISSION_DENIED = 7,          // Insufficient permissions
    NOT_FOUND = 8,                  // Resource not found (printer, file, etc.)
    ALREADY_EXISTS = 9,             // Resource already exists
    INSUFFICIENT_SPACE = 10,        // Insufficient storage space
    BAD_REQUEST = 11,               // Bad request
    CONNECTION_ERROR = 12,          // Connection error (including connection failure, loss)
    NETWORK_ERROR = 13,             // Network error
    SERVICE_UNAVAILABLE = 14,       // Service unavailable (server busy, maintenance, etc.)
    NOT_IMPLEMENTED = 15,           // Not implemented (not supported by printer or API)

    // Version compatibility errors (100-199)
    VERSION_NOT_SUPPORTED = 100,    // Version not supported
    VERSION_TOO_OLD = 101,          // Version too old
    VERSION_TOO_NEW = 102,          // Version too new

    // Authentication errors (200-299)
    UNAUTHORIZED = 200,             // Unauthorized access
    AUTHENTICATION_FAILED = 201,    // Login authentication failed
    TOKEN_EXPIRED = 202,            // Token expired
    TOKEN_INVALID = 203,            // Token invalid

    // File transfer errors (300-399)
    FILE_TRANSFER_FAILED = 300,     // File transfer failed (including upload, download)
    FILE_NOT_FOUND = 301,           // File not found

    // Printer business errors (1000-1999)
    PRINTER_BUSY = 1000,             // Printer busy
    PRINTER_OFFLINE = 1001,          // Printer offline
    PRINTER_INITIALIZATION_ERROR = 1002, // Printer initialization error
    PRINTER_COMMAND_FAILED = 1003,   // Printer command execution failed
    PRINTER_ALREADY_CONNECTED = 1004, // Printer already connected or connecting
    PRINTER_INTERNAL_ERROR = 1005,   // Printer internal error
    PRINTER_TASK_NOT_FOUND = 1006,   // Printer task not found

    HOST_TYPE_NOT_SUPPORTED = 9998,          // Host type not supported
    PRINTER_NETWORK_EXCEPTION = 9999,          // External error
};


std::string getErrorMessage(PrinterNetworkErrorCode error);

template<typename T = std::monostate>
struct PrinterNetworkResult
{
    PrinterNetworkErrorCode code = PrinterNetworkErrorCode::SUCCESS;
    std::string message = "ok";
    std::optional<T> data;

    // Default constructor
    PrinterNetworkResult() = default;

    // Perfect forwarding constructor, supports results with data
    template <typename U>
    PrinterNetworkResult(PrinterNetworkErrorCode resultCode, U &&dataVal, std::string_view msg = "") 
        : code(resultCode), data(std::forward<U>(dataVal)), message(msg.empty() ? getErrorMessage(resultCode) : msg) {}

    // Check if successful
    bool isSuccess() const noexcept
    {
        return code == PrinterNetworkErrorCode::SUCCESS;
    }

    // Check if failed
    bool isError() const noexcept
    {
        return code != PrinterNetworkErrorCode::SUCCESS;
    }

    // Check if data exists
    bool hasData() const noexcept
    {
        return data.has_value();
    }

    // Get data reference (only if data exists)
    const T& value() const&
    {
        return data.value();
    }

    T& value() &
    {
        return data.value();
    }

    T&& value() &&
    {
        return std::move(data).value();
    }

    // Functional programming support - map operation
    template <typename F>
    auto map(F&& func) const -> PrinterNetworkResult<std::invoke_result_t<F, T>>
    {
        if (isError() || !hasData()) {
            return PrinterNetworkResult<std::invoke_result_t<F, T>>::Error(code, message);
        }
        return PrinterNetworkResult<std::invoke_result_t<F, T>>::Ok(func(data.value()));
    }

    // Functional programming support - flatMap operation
    template <typename F>
    auto flatMap(F&& func) const -> std::invoke_result_t<F, T>
    {
        if (isError() || !hasData()) {
            using ReturnType = std::invoke_result_t<F, T>;
            return ReturnType::Error(code, message);
        }
        return func(data.value());
    }
};



} // namespace Slic3r

#endif 