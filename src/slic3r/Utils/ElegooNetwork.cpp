#include "ElegooNetwork.hpp"
#include "ElegooLink.hpp"
#include <wx/log.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Slic3r {

ElegooNetwork::ElegooNetwork(const PrinterNetworkInfo& printerNetworkInfo) 
    : IPrinterNetwork(printerNetworkInfo)
    , m_printerNetworkInfo(printerNetworkInfo)
{
}

ElegooNetwork::~ElegooNetwork() {
    disconnect();
}

bool ElegooNetwork::connect()
{
    return ElegooLink::getInstance()->addPrinter(m_printerNetworkInfo);
}

void ElegooNetwork::disconnect()
{
    ElegooLink::getInstance()->removePrinter(m_printerNetworkInfo);
}

bool ElegooNetwork::isConnected() const
{
    return ElegooLink::getInstance()->isPrinterConnected(m_printerNetworkInfo);
}

std::vector<PrinterNetworkInfo> ElegooNetwork::discoverDevices()   
{
    return ElegooLink::getInstance()->discoverDevices();
}

bool ElegooNetwork::sendPrintTask(const PrinterNetworkParams& params)
{
    return ElegooLink::getInstance()->sendPrintTask(m_printerNetworkInfo, params);
}

bool ElegooNetwork::sendPrintFile(const PrinterNetworkParams& params)
{
    return  ElegooLink::getInstance()->sendPrintFile(m_printerNetworkInfo, params);
}

} // namespace Slic3r 


