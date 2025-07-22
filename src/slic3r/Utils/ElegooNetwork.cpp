#include "ElegooNetwork.hpp"
#include "ElegooLocalWebSocket.hpp"
#include <wx/log.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Slic3r {

ElegooNetwork::ElegooNetwork(const PrinterInfo& printerInfo) 
    : IPrinterNetwork(printerInfo)
    , m_printerInfo(printerInfo)
{
}

ElegooNetwork::~ElegooNetwork() {
    disconnect();
}

bool ElegooNetwork::connect()
{
    return ElegooLocalWebSocket::instance()->addPrinter(m_printerInfo);
}

void ElegooNetwork::disconnect()
{
    ElegooLocalWebSocket::instance()->removePrinter(m_printerInfo);
}

bool ElegooNetwork::isConnected() const
{
    return ElegooLocalWebSocket::instance()->isPrinterConnected(m_printerInfo);
}

std::vector<PrinterInfo> ElegooNetwork::discoverDevices()   
{
    return ElegooLocalWebSocket::instance()->discoverDevices();
}

bool ElegooNetwork::sendPrintTask(const PrinterNetworkParams& params)
{
    return ElegooLocalWebSocket::instance()->sendPrintTask(m_printerInfo, params);
}

bool ElegooNetwork::sendPrintFile(const PrinterNetworkParams& params)
{
    return ElegooLocalWebSocket::instance()->sendPrintFile(m_printerInfo, params);
}

} // namespace Slic3r 


