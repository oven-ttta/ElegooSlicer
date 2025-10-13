#include "ElegooLink.hpp"
#include "ElegooPluginNetwork.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"


namespace Slic3r {

ElegooPluginNetwork::ElegooPluginNetwork(const PluginNetworkInfo& pluginNetworkInfo) : IPluginNetwork(pluginNetworkInfo) {}

ElegooPluginNetwork::~ElegooPluginNetwork(){


}

void ElegooPluginNetwork::uninit()
{
    ElegooLink::getInstance()->uninit();
}

void ElegooPluginNetwork::init()
{
    ElegooLink::getInstance()->init();
}

PrinterNetworkResult<std::string> ElegooPluginNetwork::hasInstalledPlugin()
{
    return ElegooLink::getInstance()->hasInstalledPlugin();
}

PrinterNetworkResult<bool> ElegooPluginNetwork::installPlugin(const std::string& pluginPath)
{
    return ElegooLink::getInstance()->installPlugin(pluginPath);
}   

PrinterNetworkResult<bool> ElegooPluginNetwork::uninstallPlugin()
{
    return ElegooLink::getInstance()->uninstallPlugin();
}


} // namespace Slic3r 

