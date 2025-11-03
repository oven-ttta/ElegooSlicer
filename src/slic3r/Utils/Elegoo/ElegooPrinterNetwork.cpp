#include "ElegooPrinterNetwork.hpp"
#include "ElegooLink.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include <wx/log.h>

namespace Slic3r {

ElegooPrinterNetwork::ElegooPrinterNetwork(const PrinterNetworkInfo& printerNetworkInfo) : IPrinterNetwork(printerNetworkInfo) {}

ElegooPrinterNetwork::~ElegooPrinterNetwork(){


}
void ElegooPrinterNetwork::init()
{
    ElegooLink::getInstance()->init();
}

void ElegooPrinterNetwork::uninit()
{
    ElegooLink::getInstance()->uninit();
}


PrinterNetworkResult<PrinterNetworkInfo> ElegooPrinterNetwork::connectToPrinter()
{
    return ElegooLink::getInstance()->connectToPrinter(mPrinterNetworkInfo);
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::disconnectFromPrinter()
{
    return ElegooLink::getInstance()->disconnectFromPrinter(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooPrinterNetwork::bindWANPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    return ElegooLink::getInstance()->bindWANPrinter(printerNetworkInfo);
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::unbindWANPrinter(const std::string& serialNumber)
{
    return ElegooLink::getInstance()->unbindWANPrinter(serialNumber);
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooPrinterNetwork::discoverPrinters()   
{
    return ElegooLink::getInstance()->discoverPrinters();
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::sendPrintTask(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintTask(params, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::sendPrintFile(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintFile(params, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterPrintFileResponse> ElegooPrinterNetwork::getFileDetail(const std::string& fileName)
{
    return ElegooLink::getInstance()->getFileDetail(mPrinterNetworkInfo.printerId, fileName);
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::updatePrinterName(const std::string& printerName)
{
    return ElegooLink::getInstance()->updatePrinterName(mPrinterNetworkInfo.printerId, printerName, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}
PrinterNetworkResult<PrinterMmsGroup> ElegooPrinterNetwork::getPrinterMmsInfo()
{
    return ElegooLink::getInstance()->getPrinterMmsInfo(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooPrinterNetwork::getPrinterAttributes()
{
    return ElegooLink::getInstance()->getPrinterAttributes(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooPrinterNetwork::getPrinterStatus()
{
    return ElegooLink::getInstance()->getPrinterStatus(mPrinterNetworkInfo.printerId, mPrinterNetworkInfo.networkType == NETWORK_TYPE_WAN);
}

PrinterNetworkResult<PrinterPrintFileResponse> ElegooPrinterNetwork::getFileList(int pageNumber, int pageSize)
{
    return ElegooLink::getInstance()->getFileList(mPrinterNetworkInfo.printerId, pageNumber, pageSize);
}

PrinterNetworkResult<PrinterPrintTaskResponse> ElegooPrinterNetwork::getPrintTaskList(int pageNumber, int pageSize)
{
    return ElegooLink::getInstance()->getPrintTaskList(mPrinterNetworkInfo.printerId, pageNumber, pageSize);
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::deletePrintTasks(const std::vector<std::string>& taskIds)
{
    return ElegooLink::getInstance()->deletePrintTasks(mPrinterNetworkInfo.printerId, taskIds);
}

PrinterNetworkResult<bool> ElegooPrinterNetwork::sendRtmMessage(const std::string& message)
{
    return ElegooLink::getInstance()->sendRtmMessage(mPrinterNetworkInfo.printerId, message);
}
} // namespace Slic3r 

