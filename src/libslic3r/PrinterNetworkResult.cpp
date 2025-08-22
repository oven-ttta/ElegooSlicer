#include "PrinterNetworkResult.hpp"

namespace Slic3r {

// Get localized error message based on error code
std::string getErrorMessage(PrinterNetworkErrorCode error)
{
    switch (error) {
        case PrinterNetworkErrorCode::SUCCESS:
            return _u8L("ok");            
        case PrinterNetworkErrorCode::UNKNOWN_ERROR:
            return _u8L("unknown error occurred");
        case PrinterNetworkErrorCode::INTERNAL_ERROR:
            return _u8L("internal error occurred");
        case PrinterNetworkErrorCode::INVALID_PARAMETER:
            return _u8L("invalid parameter provided");
        case PrinterNetworkErrorCode::INVALID_FORMAT:
            return _u8L("invalid data format");
        case PrinterNetworkErrorCode::TIMEOUT:
            return _u8L("operation timed out");
        case PrinterNetworkErrorCode::CANCELLED:
            return _u8L("operation was cancelled");
        case PrinterNetworkErrorCode::ACCESS_DENIED:
            return _u8L("permission denied");
        case PrinterNetworkErrorCode::NOT_FOUND:
            return _u8L("resource not found");
        case PrinterNetworkErrorCode::ALREADY_EXISTS:
            return _u8L("resource already exists");
        case PrinterNetworkErrorCode::INSUFFICIENT_SPACE:
            return _u8L("insufficient storage space");
        case PrinterNetworkErrorCode::BAD_REQUEST:
            return _u8L("bad request");
        case PrinterNetworkErrorCode::CONNECTION_ERROR:
            return _u8L("connection error occurred");
        case PrinterNetworkErrorCode::NETWORK_ERROR:
            return _u8L("network error occurred");
        case PrinterNetworkErrorCode::SERVICE_UNAVAILABLE:
            return _u8L("service is currently unavailable");
        case PrinterNetworkErrorCode::NOT_IMPLEMENTED:
            return _u8L("feature not implemented");
            
        case PrinterNetworkErrorCode::VERSION_NOT_SUPPORTED:
            return _u8L("version not supported");
        case PrinterNetworkErrorCode::VERSION_TOO_OLD:
            return _u8L("version is too old");
        case PrinterNetworkErrorCode::VERSION_TOO_NEW:
            return _u8L("version is too new");
            
        case PrinterNetworkErrorCode::NOT_AUTHORIZED:
            return _u8L("unauthorized access");
        case PrinterNetworkErrorCode::INVALID_CREDENTIAL:
            return _u8L("authentication failed");
        case PrinterNetworkErrorCode::TOKEN_EXPIRED:
            return _u8L("authentication token expired");
            
        case PrinterNetworkErrorCode::FILE_TRANSFER_FAILED:
            return _u8L("file transfer failed");
        case PrinterNetworkErrorCode::FILE_NOT_FOUND:
            return _u8L("file not found");
            
        case PrinterNetworkErrorCode::PRINTER_BUSY:
            return _u8L("printer is currently busy");
        case PrinterNetworkErrorCode::PRINTER_OFFLINE:
            return _u8L("printer is offline");
        case PrinterNetworkErrorCode::PRINTER_INITIALIZATION_ERROR:
            return _u8L("printer initialization error");
        case PrinterNetworkErrorCode::PRINTER_COMMAND_FAILED:
            return _u8L("printer command execution failed");
        case PrinterNetworkErrorCode::PRINTER_ALREADY_CONNECTED:
            return _u8L("printer is already connected");
        case PrinterNetworkErrorCode::PRINTER_INTERNAL_ERROR:
            return _u8L("printer internal error");
        case PrinterNetworkErrorCode::PRINTER_TASK_NOT_FOUND:
            return _u8L("printer task not found");
        case PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION:
            return _u8L("printer network exception");
        case PrinterNetworkErrorCode::PRINTER_NETWORK_INVALID_DATA:
            return _u8L("printer network invalid data");
        case PrinterNetworkErrorCode::PRINTER_MMS_NOT_CONNECTED:
            return _u8L("printer MMS not connected");
        case PrinterNetworkErrorCode::PRINTER_NOT_FOUND:
            return _u8L("printer not found");
        case PrinterNetworkErrorCode::PRINTER_ALREADY_EXISTS:
            return _u8L("printer already exists");
        case PrinterNetworkErrorCode::PRINTER_TYPE_NOT_SUPPORTED:
            return _u8L("printer type not supported");
        case PrinterNetworkErrorCode::HOST_TYPE_NOT_SUPPORTED:
            return _u8L("host type not supported");
        default:
            return _u8L("unknown error occurred");
    }
}

} // namespace Slic3r 