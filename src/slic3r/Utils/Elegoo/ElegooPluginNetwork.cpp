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

PrinterNetworkResult<PluginNetworkInfo> ElegooPluginNetwork::hasInstalledPlugin()
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

PrinterNetworkResult<PluginNetworkInfo> ElegooPluginNetwork::getPluginLastestVersion()
{
    PluginNetworkInfo pluginNetworkInfo;
    pluginNetworkInfo.version = "1.0.0";
    pluginNetworkInfo.downloadUrl = "https://www.elegoo.com/download/plugin/1.0.0";
    pluginNetworkInfo.hostType = "elegoo";
    return PrinterNetworkResult<PluginNetworkInfo>(PrinterNetworkErrorCode::SUCCESS, pluginNetworkInfo);
}

PrinterNetworkResult<std::vector<PluginNetworkInfo>> ElegooPluginNetwork::getPluginOldVersions()
{
    std::vector<PluginNetworkInfo> pluginNetworkInfos;
    PluginNetworkInfo pluginNetworkInfo;
    pluginNetworkInfo.version = "1.0.0";
    pluginNetworkInfo.downloadUrl = "https://www.elegoo.com/download/plugin/1.0.0";
    pluginNetworkInfo.hostType = "elegoo";
    pluginNetworkInfos.push_back(pluginNetworkInfo);
    return PrinterNetworkResult<std::vector<PluginNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, pluginNetworkInfos);
}

} // namespace Slic3r 

