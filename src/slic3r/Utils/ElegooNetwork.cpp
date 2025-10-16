#include "ElegooNetwork.hpp"
#include "ElegooLink.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include <wx/log.h>

namespace Slic3r {

ElegooNetwork::ElegooNetwork(const PrinterNetworkInfo& printerNetworkInfo) : IPrinterNetwork(printerNetworkInfo) {}

ElegooNetwork::~ElegooNetwork(){


}


void ElegooNetwork::init()
{
    ElegooLink::getInstance()->init();
}

void ElegooNetwork::uninit()
{
    ElegooLink::getInstance()->uninit();
}


PrinterNetworkResult<PrinterNetworkInfo> ElegooNetwork::connectToPrinter()
{
    return ElegooLink::getInstance()->connectToPrinter(mPrinterNetworkInfo);
}

PrinterNetworkResult<bool> ElegooNetwork::disconnectFromPrinter()
{
    return ElegooLink::getInstance()->disconnectFromPrinter(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooNetwork::bindWANPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    return ElegooLink::getInstance()->bindWANPrinter(printerNetworkInfo);
}

PrinterNetworkResult<bool> ElegooNetwork::unbindWANPrinter(const std::string& printerId)
{
    return ElegooLink::getInstance()->unbindWANPrinter(printerId);
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooNetwork::discoverPrinters()   
{
    return ElegooLink::getInstance()->discoverPrinters();
}
PrinterNetworkResult<bool> ElegooNetwork::sendPrintTask(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintTask(params, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);

}
PrinterNetworkResult<bool> ElegooNetwork::sendPrintFile(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintFile(params, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);

}
PrinterNetworkResult<PrinterPrintFileResponse> ElegooNetwork::getFileDetail(const std::string& fileName)
{
    return ElegooLink::getInstance()->getFileDetail(mPrinterNetworkInfo.printerId, fileName);
}

PrinterNetworkResult<PrinterMmsGroup> ElegooNetwork::getPrinterMmsInfo()
{
    return ElegooLink::getInstance()->getPrinterMmsInfo(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooNetwork::getPrinterAttributes()
{
    return ElegooLink::getInstance()->getPrinterAttributes(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterPrintFileResponse> ElegooNetwork::getFileList(int pageNumber, int pageSize)
{
    return ElegooLink::getInstance()->getFileList(mPrinterNetworkInfo.printerId, pageNumber, pageSize);
}

PrinterNetworkResult<PrinterPrintTaskResponse> ElegooNetwork::getPrintTaskList(int pageNumber, int pageSize)
{
    return ElegooLink::getInstance()->getPrintTaskList(mPrinterNetworkInfo.printerId, pageNumber, pageSize);
}

PrinterNetworkResult<bool> ElegooNetwork::deletePrintTasks(const std::vector<std::string>& taskIds)
{
    return ElegooLink::getInstance()->deletePrintTasks(mPrinterNetworkInfo.printerId, taskIds);
}

PrinterNetworkResult<bool> ElegooNetwork::sendRtmMessage(const std::string& message)
{
    return ElegooLink::getInstance()->sendRtmMessage(mPrinterNetworkInfo.printerId, message);
}

} // namespace Slic3r 

