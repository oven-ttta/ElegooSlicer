#include "PrinterPluginManager.hpp"
#include "PrintHost.hpp"
#include "libslic3r/Utils.hpp"

namespace Slic3r {


PrinterPluginManager::PrinterPluginManager(){ }
PrinterPluginManager::~PrinterPluginManager() { }


bool PrinterPluginManager::init() {

    std::vector<std::string> hostTypeList = getPluginList();
    for(const std::string& hostType : hostTypeList) {
       installPlugin(hostType);
    }

    return true;
}

bool PrinterPluginManager::uninit() {
    std::vector<std::string> hostTypeList = getPluginList();
    for(const std::string& hostType : hostTypeList) {
        uninstallPlugin(hostType);
    }
    return true;
}

PrinterNetworkResult<std::string> PrinterPluginManager::hasInstalledPlugin(const std::string& hostTypeStr) {
 
    PrinterNetworkInfo printer;
    printer.hostType = hostTypeStr;
    std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printer);
    if(network) {
        return network->hasInstalledPlugin();
    }
    return PrinterNetworkResult<std::string>(PrinterNetworkErrorCode::HOST_TYPE_NOT_SUPPORTED, "");
}

PrinterNetworkResult<bool> PrinterPluginManager::installPlugin(const std::string& hostTypeStr) {
    
    std::string library;
    std::string dataDirStr = data_dir();
    boost::filesystem::path dataDirPath(dataDirStr);
    auto pluginFolderPath = dataDirPath / "plugins" / hostTypeStr;

    PrinterNetworkInfo printer;
    printer.hostType = hostTypeStr;
    std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printer);
    if(network) {
        return network->installPlugin(pluginFolderPath.string());
    }
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::HOST_TYPE_NOT_SUPPORTED, false);
}

PrinterNetworkResult<bool> PrinterPluginManager::uninstallPlugin(const std::string& hostTypeStr) {
    PrinterNetworkInfo printer;
    printer.hostType = hostTypeStr;
    std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printer);
    if(network) {
        return network->uninstallPlugin();
    }
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}

std::vector<std::string> PrinterPluginManager::getPluginList() {
    std::vector<std::string> pluginList;
    for (const auto& hostType : {htElegooLink}) {    
        pluginList.push_back(PrintHost::get_print_host_type_str(hostType));
    }
    return pluginList;
}

} // namespace Slic3r
