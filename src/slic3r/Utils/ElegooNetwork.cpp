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

PrinterNetworkResult<std::vector<PrinterPrintFile>> ElegooNetwork::getFileList(const std::string& printerId, int pageNumber, int pageSize)
{
    return ElegooLink::getInstance()->getFileList(printerId, pageNumber, pageSize);
}

PrinterNetworkResult<std::vector<PrinterPrintTask>> ElegooNetwork::getPrintTaskList(const std::string& printerId, int pageNumber, int pageSize)
{
    return ElegooLink::getInstance()->getPrintTaskList(printerId, pageNumber, pageSize);
}

PrinterNetworkResult<bool> ElegooNetwork::deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds)
{
    return ElegooLink::getInstance()->deletePrintTasks(printerId, taskIds);
}

void ElegooNetwork::uninit()
{
    ElegooLink::getInstance()->uninit();
}

void ElegooNetwork::init()
{
    ElegooLink::getInstance()->init();
}

PrinterNetworkResult<std::string> ElegooNetwork::hasInstalledPlugin()
{
    return ElegooLink::getInstance()->hasInstalledPlugin();
}

PrinterNetworkResult<bool> ElegooNetwork::installPlugin(const std::string& pluginPath)
{
    return ElegooLink::getInstance()->installPlugin(pluginPath);
}   

PrinterNetworkResult<bool> ElegooNetwork::uninstallPlugin()
{
    return ElegooLink::getInstance()->uninstallPlugin();
}

PrinterNetworkResult<bool> ElegooNetwork::loginWAN(const NetworkUserInfo& userInfo)
{
    return ElegooLink::getInstance()->loginWAN(userInfo);
}

PrinterNetworkResult<NetworkUserInfo> ElegooNetwork::getRtcToken()
{
    return ElegooLink::getInstance()->getRtcToken();
}

PrinterNetworkResult<bool> ElegooNetwork::sendRtmMessage(const std::string& printerId, const std::string& message)
{
    return ElegooLink::getInstance()->sendRtmMessage(printerId, message);
}

} // namespace Slic3r 

