#include "PrinterNetwork.hpp"
#include "ElegooNetwork.hpp"
#include "libslic3r/PrintConfig.hpp"
#include <wx/log.h>

namespace Slic3r {


std::unique_ptr<IPrinterNetwork> PrinterNetworkFactory::createNetwork(const PrintHostType hostType) {
  switch (hostType) {
    case htElegooLink: return std::make_unique<ElegooNetwork>();
    default: return nullptr;
  }
  return nullptr;
}


} // namespace Slic3r