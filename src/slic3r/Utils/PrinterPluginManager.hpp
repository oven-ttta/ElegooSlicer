#pragma once
#include <map>
#include "slic3r/Utils/PrinterNetwork.hpp"
#include "slic3r/Utils/Singleton.hpp"

namespace Slic3r { 


class PrinterPluginManager : public Singleton<PrinterPluginManager>
{
    friend class Singleton<PrinterPluginManager>;
public:
    ~PrinterPluginManager();
    PrinterPluginManager(const PrinterPluginManager&) = delete;
    PrinterPluginManager& operator=(const PrinterPluginManager&) = delete;

    bool init();
    bool uninit();


    PrinterNetworkResult<std::string> hasInstalledPlugin(const std::string& hostTypeStr);
    PrinterNetworkResult<bool> installPlugin(const std::string& hostTypeStr);
    PrinterNetworkResult<bool> uninstallPlugin(const std::string& hostTypeStr);
    std::vector<std::string> getPluginList();


private:
    PrinterPluginManager();



};
} // namespace Slic3r::GUI 
