#ifndef slic3r_ElegooPluginNetwork_hpp_
#define slic3r_ElegooPluginNetwork_hpp_

#include "PrinterNetwork.hpp"

namespace Slic3r {

class ElegooPluginNetwork : public IPluginNetwork
{
public:
    ElegooPluginNetwork(const PluginNetworkInfo& pluginNetworkInfo);
    ElegooPluginNetwork()                                      = delete;
    ElegooPluginNetwork(const ElegooPluginNetwork&)            = delete;
    ElegooPluginNetwork& operator=(const ElegooPluginNetwork&) = delete;
    virtual ~ElegooPluginNetwork();

    virtual PrinterNetworkResult<std::string> hasInstalledPlugin() override;
    virtual PrinterNetworkResult<bool>        installPlugin(const std::string& pluginPath) override;
    virtual PrinterNetworkResult<bool>        uninstallPlugin() override;

    virtual void init() override;
    virtual void uninit() override;
};

} // namespace Slic3r

#endif
