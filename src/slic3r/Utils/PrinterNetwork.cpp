#include "PrinterNetwork.hpp"
#include "ElegooNetwork.hpp"
#include <wx/log.h>
#include "PrintHost.hpp"
namespace Slic3r {

std::shared_ptr<IPrinterNetwork> PrinterNetworkFactory::createNetwork(const PrinterNetworkInfo& printerNetworkInfo) {
    switch (PrintHost::get_print_host_type(printerNetworkInfo.hostType)) {
        case PrintHostType::htElegooLink: 
            return std::make_shared<ElegooNetwork>(printerNetworkInfo);
            default: 
                wxLogWarning("Unsupported network type: %s", printerNetworkInfo.hostType.c_str());
            return nullptr;
    }
    return nullptr;
}

} // namespace Slic3r