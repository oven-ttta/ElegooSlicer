#include "PrinterNetwork.hpp"
#include "ElegooNetwork.hpp"
#include "libslic3r/PrintConfig.hpp"
#include <wx/log.h>

namespace Slic3r {

IPrinterNetwork::IPrinterNetwork(const PrinterNetworkInfo& printerNetworkInfo) {
    m_printerNetworkInfo.id = printerNetworkInfo.id;
    m_printerNetworkInfo.name = printerNetworkInfo.name;
    m_printerNetworkInfo.ip = printerNetworkInfo.ip;
    m_printerNetworkInfo.port = printerNetworkInfo.port;
    m_printerNetworkInfo.vendor = printerNetworkInfo.vendor;
    m_printerNetworkInfo.machineName = printerNetworkInfo.machineName;
    m_printerNetworkInfo.machineModel = printerNetworkInfo.machineModel;
    m_printerNetworkInfo.protocolVersion = printerNetworkInfo.protocolVersion;
    m_printerNetworkInfo.firmwareVersion = printerNetworkInfo.firmwareVersion;
    m_printerNetworkInfo.deviceId = printerNetworkInfo.deviceId;
    m_printerNetworkInfo.deviceType = printerNetworkInfo.deviceType;
    m_printerNetworkInfo.serialNumber = printerNetworkInfo.serialNumber;
    m_printerNetworkInfo.webUrl = printerNetworkInfo.webUrl;
    m_printerNetworkInfo.connectionUrl = printerNetworkInfo.connectionUrl;
}



std::unique_ptr<IPrinterNetwork> PrinterNetworkFactory::createNetwork(const PrinterNetworkInfo& printerNetworkInfo, const PrintHostType hostType) {
  switch (hostType) {
    case htElegooLink: return std::make_unique<ElegooNetwork>(printerNetworkInfo);
    default: return nullptr;
  }
  return nullptr;
}


} // namespace Slic3r