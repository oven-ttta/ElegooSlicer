#include "ElegooNetwork.hpp"
#include "ElegooLink.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include <wx/log.h>

namespace Slic3r {

ElegooNetwork::ElegooNetwork() = default;

ElegooNetwork::~ElegooNetwork() = default;

PrinterNetworkResult<bool> ElegooNetwork::addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected)
{
    if (printerNetworkInfo.printerId.empty()) {
        wxLogError("Invalid printer ID provided to ElegooNetwork::addPrinter");
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::INVALID_PARAMETER, false);
    }
    return ElegooLink::getInstance()->addPrinter(printerNetworkInfo, connected);

}

PrinterNetworkResult<bool> ElegooNetwork::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    if (printerNetworkInfo.printerId.empty()) {
        wxLogError("Invalid printer ID provided to ElegooNetwork::connectToPrinter");
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::INVALID_PARAMETER, false);
    }
    return ElegooLink::getInstance()->connectToPrinter(printerNetworkInfo);
}

PrinterNetworkResult<bool> ElegooNetwork::disconnectFromPrinter(const std::string& printerId)
{
    if (printerId.empty()) {
        wxLogError("Invalid printer ID provided to ElegooNetwork::disconnectFromPrinter");
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::INVALID_PARAMETER, false);
    }
    return ElegooLink::getInstance()->disconnectFromPrinter(printerId);
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooNetwork::discoverDevices()   
{
    return ElegooLink::getInstance()->discoverDevices();
}

PrinterNetworkResult<bool> ElegooNetwork::sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params)
{
    if (printerNetworkInfo.printerId.empty()) {
        wxLogError("Invalid printer ID provided to ElegooNetwork::sendPrintTask");
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::INVALID_PARAMETER, false);
    }
    return ElegooLink::getInstance()->sendPrintTask(printerNetworkInfo, params);

}

PrinterNetworkResult<bool> ElegooNetwork::sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params)
{
    if (printerNetworkInfo.printerId.empty()) {
        wxLogError("Invalid printer ID provided to ElegooNetwork::sendPrintFile");
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::INVALID_PARAMETER, false);
    }
    return ElegooLink::getInstance()->sendPrintFile(printerNetworkInfo, params);

}

} // namespace Slic3r 

