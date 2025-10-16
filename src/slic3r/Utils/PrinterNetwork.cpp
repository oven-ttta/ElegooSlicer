#include "PrinterNetwork.hpp"
#include "ElegooNetwork.hpp"
#include "ElegooUserNetwork.hpp"
#include "ElegooPluginNetwork.hpp"
#include <wx/log.h>
#include "PrintHost.hpp"
namespace Slic3r {

std::shared_ptr<IPrinterNetwork> NetworkFactory::createPrinterNetwork(const PrinterNetworkInfo& printerNetworkInfo)
{
    switch (PrintHost::get_print_host_type(printerNetworkInfo.hostType)) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooNetwork>(printerNetworkInfo);
    }
    default: {
        wxLogWarning("Unsupported network type: %s", printerNetworkInfo.hostType.c_str());
        return nullptr;
    }
    }
    return nullptr;
}

std::shared_ptr<IUserNetwork> NetworkFactory::createUserNetwork(const UserNetworkInfo& userNetworkInfo)
{
    switch (PrintHost::get_print_host_type(userNetworkInfo.hostType)) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooUserNetwork>(userNetworkInfo);
    }
    default: {wxLogWarning("Unsupported network type: %s", userNetworkInfo.hostType.c_str()); return nullptr;}
    }
    return nullptr;
}

std::shared_ptr<IPluginNetwork> NetworkFactory::createPluginNetwork(const PluginNetworkInfo& pluginNetworkInfo)
{
    switch (PrintHost::get_print_host_type(pluginNetworkInfo.hostType)) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooPluginNetwork>(pluginNetworkInfo);
    }
    default: {wxLogWarning("Unsupported network type: %s", pluginNetworkInfo.hostType.c_str()); return nullptr;}
    }
    return nullptr;
}

void NetworkFactory::initNetwork()
{
    ElegooNetwork::init();
    ElegooUserNetwork::init();
    ElegooPluginNetwork::init();
}

void NetworkFactory::uninitNetwork()
{
    ElegooNetwork::uninit();
    ElegooUserNetwork::uninit();
    ElegooPluginNetwork::uninit();
}

} // namespace Slic3r