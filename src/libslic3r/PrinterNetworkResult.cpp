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
            return _u8L("An unknown error occurred. Please try again later.");
        case PrinterNetworkErrorCode::NOT_INITIALIZED:
            return _u8L("Not initialized. Please try again later.");
        case PrinterNetworkErrorCode::INVALID_PARAMETER:
            return _u8L("Invalid parameter. Please try again.");
        case PrinterNetworkErrorCode::OPERATION_TIMEOUT:
            return _u8L("Operation timed out. Please try again.");
        case PrinterNetworkErrorCode::OPERATION_CANCELLED:
            return _u8L("Operation cancelled");
        case PrinterNetworkErrorCode::PRINTER_ACCESS_DENIED:
            return _u8L("Access denied. Please try again later.");
        case PrinterNetworkErrorCode::PRINTER_MISSING_BED_LEVELING_DATA:
            return _u8L("Missing bed leveling data. Please level the bed.");
        case PrinterNetworkErrorCode::PRINTER_PRINT_FILE_NOT_FOUND:
            return _u8L("Print file not found. Please troubleshoot.");
        case PrinterNetworkErrorCode::PRINTER_CONNECTION_ERROR:
            return _u8L("Connection failed. Please check the network of your computer and the printer, then try again.");
        case PrinterNetworkErrorCode::NETWORK_ERROR:
            return _u8L("Network error occurred. Please check the network of your computer and the printer, then try again.");
        case PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD:
            return _u8L("Invalid username or password. Please check and try again.");
        case PrinterNetworkErrorCode::INVALID_TOKEN:
            return _u8L("Invalid token. Please check and try again.");
        case PrinterNetworkErrorCode::INVALID_ACCESS_CODE:
            return _u8L("Invalid access code. Please check and try again.");
        case PrinterNetworkErrorCode::INVALID_PIN_CODE:
            return _u8L("Invalid PIN code. Please check and try again.");
        case PrinterNetworkErrorCode::PRINTER_CONNECTION_LIMIT_EXCEEDED:
            return _u8L("Connection limit reached. Please check and try again.");
        case PrinterNetworkErrorCode::FILE_TRANSFER_FAILED:
            return _u8L("File upload failed. Please try again later.");
        case PrinterNetworkErrorCode::FILE_NOT_FOUND:
            return _u8L("File not found. Please troubleshoot.");        
        case PrinterNetworkErrorCode::PRINTER_BUSY:
            return _u8L("Printer is busy. Please try again later.");
        case PrinterNetworkErrorCode::PRINTER_COMMAND_FAILED:
            return _u8L("Operation failed. Please try again later.");
        case PrinterNetworkErrorCode::PRINTER_ALREADY_CONNECTED:
            return _u8L("Printer already connected");
        case PrinterNetworkErrorCode::PRINTER_UNKNOWN_ERROR:
            return _u8L("An unknown error occurred. Please try again later.");
        case PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION:
            return _u8L("Printer network error. Please try again later or restart the printer.");
        case PrinterNetworkErrorCode::PRINTER_NETWORK_INVALID_DATA:
            return _u8L("Invalid printer data received. Please try again later or restart the printer.");
        case PrinterNetworkErrorCode::PRINTER_MMS_NOT_CONNECTED:
            return _u8L("Printer MMS connection failed");
        case PrinterNetworkErrorCode::PRINTER_NOT_FOUND:
            return _u8L("Printer not found. Please refresh and try again.");
        case PrinterNetworkErrorCode::PRINTER_ALREADY_EXISTS:
            return _u8L("Printer already connected");
        case PrinterNetworkErrorCode::PRINTER_TYPE_NOT_SUPPORTED:
            return _u8L("Printer type not supported");
        case PrinterNetworkErrorCode::HOST_TYPE_NOT_SUPPORTED:
            return _u8L("Host type not supported");
        case PrinterNetworkErrorCode::CREATE_NETWORK_FOR_HOST_TYPE_FAILED:
            return _u8L("Failed to establish network connection");
        case PrinterNetworkErrorCode::PRINTER_NOT_SELECTED:
            return _u8L("Printer not selected. Please try again.");
        case PrinterNetworkErrorCode::PRINTER_MMS_FILAMENT_NOT_MAPPED:
            return _u8L("Unmapped filament detected, printing cannot be started.");
        case PrinterNetworkErrorCode::PRINTER_HOST_NOT_MATCH:
            return _u8L("The IP address does not match the printer's information. Please remove the printer and add it again.");
        default:
            return _u8L("An unknown error occurred. Please try again later.");
    }
}

} // namespace Slic3r 