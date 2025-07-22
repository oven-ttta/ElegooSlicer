#include "PrinterNetwork.hpp"
#include "ElegooNetwork.hpp"
#include "libslic3r/PrintConfig.hpp"
#include <wx/log.h>

namespace Slic3r {

IPrinterNetwork::IPrinterNetwork(const PrinterInfo& printerInfo) {
    m_printerInfo.id = printerInfo.id;
    m_printerInfo.name = printerInfo.name;
    m_printerInfo.ip = printerInfo.ip;
    m_printerInfo.port = printerInfo.port;
    m_printerInfo.vendor = printerInfo.vendor;
    m_printerInfo.machineName = printerInfo.machineName;
    m_printerInfo.machineModel = printerInfo.machineModel;
    m_printerInfo.protocolVersion = printerInfo.protocolVersion;
    m_printerInfo.firmwareVersion = printerInfo.firmwareVersion;
    m_printerInfo.deviceId = printerInfo.deviceId;
    m_printerInfo.deviceType = printerInfo.deviceType;
    m_printerInfo.serialNumber = printerInfo.serialNumber;
    m_printerInfo.webUrl = printerInfo.webUrl;
    m_printerInfo.connectionUrl = printerInfo.connectionUrl;
}



std::unique_ptr<IPrinterNetwork> PrinterNetworkFactory::createNetwork(const PrinterInfo& printerInfo, const PrintHostType hostType) {
  switch (hostType) {
    case htElegooLink: return std::make_unique<ElegooNetwork>(printerInfo);
    default: return nullptr;
  }
  return nullptr;
}


} // namespace Slic3r