#include "PrinterNetworkResult.hpp"
#include "I18N.hpp"
namespace Slic3r {

// Get localized error message based on error code
std::string getErrorMessage(PrinterNetworkErrorCode error)
{
    switch (error) {
        case PrinterNetworkErrorCode::SUCCESS:
            return _u8L("Success");            
        case PrinterNetworkErrorCode::UNKNOWN_ERROR:
            return _u8L("An unknown error occurred");
        case PrinterNetworkErrorCode::INTERNAL_ERROR:
            return _u8L("An internal error occurred");
        case PrinterNetworkErrorCode::INVALID_PARAMETER:
            return _u8L("Invalid parameter provided");
        case PrinterNetworkErrorCode::INVALID_FORMAT:
            return _u8L("Invalid data format");
        case PrinterNetworkErrorCode::TIMEOUT:
            return _u8L("Operation timed out");
        case PrinterNetworkErrorCode::CANCELLED:
            return _u8L("Operation was cancelled");
        case PrinterNetworkErrorCode::ACCESS_DENIED:
            return _u8L("Access denied");
        case PrinterNetworkErrorCode::NOT_FOUND:
            return _u8L("Resource not found");
        case PrinterNetworkErrorCode::ALREADY_EXISTS:
            return _u8L("Resource already exists");
        case PrinterNetworkErrorCode::INSUFFICIENT_SPACE:
            return _u8L("Insufficient storage space");
        case PrinterNetworkErrorCode::BAD_REQUEST:
            return _u8L("Bad request");
        case PrinterNetworkErrorCode::CONNECTION_ERROR:
            return _u8L("Connection failed");
        case PrinterNetworkErrorCode::NETWORK_ERROR:
            return _u8L("Network error occurred");
        case PrinterNetworkErrorCode::SERVICE_UNAVAILABLE:
            return _u8L("Service temporarily unavailable");
        case PrinterNetworkErrorCode::NOT_IMPLEMENTED:
            return _u8L("Feature not available");      
        case PrinterNetworkErrorCode::VERSION_NOT_SUPPORTED:
            return _u8L("Version not supported");
        case PrinterNetworkErrorCode::VERSION_TOO_OLD:
            return _u8L("Version is too old");
        case PrinterNetworkErrorCode::VERSION_TOO_NEW:
            return _u8L("Version is too new");        
        case PrinterNetworkErrorCode::NOT_AUTHORIZED:
            return _u8L("Access unauthorized");
        case PrinterNetworkErrorCode::INVALID_CREDENTIAL:
            return _u8L("Authentication failed");
        case PrinterNetworkErrorCode::TOKEN_EXPIRED:
            return _u8L("Login session expired");        
        case PrinterNetworkErrorCode::NOT_AUTHENTICATED:
            return _u8L("Not authenticated");
        case PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD:
            return _u8L("Invalid username or password");
        case PrinterNetworkErrorCode::INVALID_TOKEN:
            return _u8L("Invalid token");
        case PrinterNetworkErrorCode::INVALID_ACCESS_CODE:
            return _u8L("Invalid access code");
        case PrinterNetworkErrorCode::INVALID_PIN_CODE:
            return _u8L("Invalid PIN code");
        case PrinterNetworkErrorCode::PRINTER_CONNECTION_LIMIT_EXCEEDED:
            return _u8L("Connection limit exceeded");
        case PrinterNetworkErrorCode::FILE_TRANSFER_FAILED:
            return _u8L("File upload failed");
        case PrinterNetworkErrorCode::FILE_NOT_FOUND:
            return _u8L("File not found");        
        case PrinterNetworkErrorCode::PRINTER_BUSY:
            return _u8L("Printer is busy");
        case PrinterNetworkErrorCode::PRINTER_OFFLINE:
            return _u8L("Printer is offline");
        case PrinterNetworkErrorCode::PRINTER_INITIALIZATION_ERROR:
            return _u8L("Printer initialization failed");
        case PrinterNetworkErrorCode::PRINTER_COMMAND_FAILED:
            return _u8L("Printer command failed");
        case PrinterNetworkErrorCode::PRINTER_ALREADY_CONNECTED:
            return _u8L("Printer is already connected");
        case PrinterNetworkErrorCode::PRINTER_INTERNAL_ERROR:
            return _u8L("Printer internal error");
        case PrinterNetworkErrorCode::PRINTER_TASK_NOT_FOUND:
            return _u8L("Print task not found");
        case PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION:
            return _u8L("Printer network error");
        case PrinterNetworkErrorCode::PRINTER_NETWORK_INVALID_DATA:
            return _u8L("Invalid printer data received");
        case PrinterNetworkErrorCode::PRINTER_MMS_NOT_CONNECTED:
            return _u8L("Printer MMS connection failed");
        case PrinterNetworkErrorCode::PRINTER_NOT_FOUND:
            return _u8L("Printer not found");
        case PrinterNetworkErrorCode::PRINTER_ALREADY_EXISTS:
            return _u8L("Printer already exists");
        case PrinterNetworkErrorCode::PRINTER_TYPE_NOT_SUPPORTED:
            return _u8L("Printer type not supported");
        case PrinterNetworkErrorCode::HOST_TYPE_NOT_SUPPORTED:
            return _u8L("Host type not supported");
        case PrinterNetworkErrorCode::CREATE_NETWORK_FOR_HOST_TYPE_FAILED:
            return _u8L("Failed to create network connection");
        case PrinterNetworkErrorCode::PRINTER_NOT_SELECTED:
            return _u8L("Printer not selected");
        case PrinterNetworkErrorCode::PRINTER_MMS_FILAMENT_NOT_MAPPED:
            return _u8L("Some filaments are not mapped");
        default:
            return _u8L("An unknown error occurred");
    }
}

} // namespace Slic3r 