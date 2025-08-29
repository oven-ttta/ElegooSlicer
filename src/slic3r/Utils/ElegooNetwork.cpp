#include "ElegooNetwork.hpp"
#include "ElegooLink.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include <wx/log.h>

namespace Slic3r {

ElegooNetwork::ElegooNetwork(const PrinterNetworkInfo& printerNetworkInfo) : IPrinterNetwork(printerNetworkInfo) {}

ElegooNetwork::~ElegooNetwork(){


}

PrinterNetworkResult<PrinterNetworkInfo> ElegooNetwork::connectToPrinter()
{
    return ElegooLink::getInstance()->connectToPrinter(mPrinterNetworkInfo);
}

PrinterNetworkResult<bool> ElegooNetwork::disconnectFromPrinter()
{
    return ElegooLink::getInstance()->disconnectFromPrinter(mPrinterNetworkInfo.printerId);
}
PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooNetwork::discoverPrinters()   
{
    return ElegooLink::getInstance()->discoverPrinters();
}
PrinterNetworkResult<bool> ElegooNetwork::sendPrintTask(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintTask(params);

}
PrinterNetworkResult<bool> ElegooNetwork::sendPrintFile(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintFile(params);

}


PrinterNetworkResult<PrinterMmsGroup> ElegooNetwork::getPrinterMmsInfo()
{
    return ElegooLink::getInstance()->getPrinterMmsInfo(mPrinterNetworkInfo.printerId);
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooNetwork::getPrinterAttributes()
{
    return ElegooLink::getInstance()->getPrinterAttributes(mPrinterNetworkInfo.printerId);
}

void ElegooNetwork::uninit()
{
    ElegooLink::getInstance()->uninit();
}

void ElegooNetwork::init()
{
    ElegooLink::getInstance()->init();
}

} // namespace Slic3r 

