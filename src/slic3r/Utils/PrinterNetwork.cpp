#include "PrinterNetwork.hpp"
#include "ElegooNetwork.hpp"
#include <wx/log.h>

namespace Slic3r {

std::shared_ptr<IPrinterNetwork> PrinterNetworkFactory::createNetwork(const PrintHostType hostType) {
    switch (hostType) {
        case htElegooLink: 
            return std::make_shared<ElegooNetwork>();
        default: 
            wxLogWarning("Unsupported network type: %d", static_cast<int>(hostType));
            return nullptr;
    }
    return nullptr;
}

} // namespace Slic3r