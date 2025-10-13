#pragma once
#include <map>
#include "slic3r/Utils/Singleton.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"

namespace Slic3r { 


class PrinterPluginManager : public Singleton<PrinterPluginManager>
{
    friend class Singleton<PrinterPluginManager>;
public:
    ~PrinterPluginManager();
    PrinterPluginManager(const PrinterPluginManager&) = delete;
    PrinterPluginManager& operator=(const PrinterPluginManager&) = delete;

    PrinterNetworkResult<std::string> hasInstalledPlugin(const std::string& hostTypeStr);
    PrinterNetworkResult<bool> installPlugin(const std::string& hostTypeStr);
    PrinterNetworkResult<bool> uninstallPlugin(const std::string& hostTypeStr);
    std::vector<std::string> getPluginList();

    bool init();
    bool uninit();


private:
    PrinterPluginManager();


};
} // namespace Slic3r::GUI 
