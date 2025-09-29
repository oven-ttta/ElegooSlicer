#ifndef slic3r_ElegooNetwork_hpp_
#define slic3r_ElegooNetwork_hpp_

#include "PrinterNetwork.hpp"

namespace Slic3r {

class ElegooNetwork : public IPrinterNetwork
{
public:
    ElegooNetwork(const PrinterNetworkInfo& printerNetworkInfo);
    ElegooNetwork()=delete;
    ElegooNetwork(const ElegooNetwork&)=delete;
    ElegooNetwork& operator=(const ElegooNetwork&)=delete;
    virtual ~ElegooNetwork();
    virtual PrinterNetworkResult<PrinterNetworkInfo> connectToPrinter() override;
    virtual PrinterNetworkResult<bool> disconnectFromPrinter() override;
    virtual PrinterNetworkResult<bool> sendPrintTask(const PrinterNetworkParams& params) override;
    virtual PrinterNetworkResult<bool> sendPrintFile(const PrinterNetworkParams& params) override;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> discoverPrinters() override;
    virtual PrinterNetworkResult<PrinterMmsGroup> getPrinterMmsInfo() override;
    virtual PrinterNetworkResult<PrinterNetworkInfo> getPrinterAttributes() override;  
    virtual PrinterNetworkResult<std::vector<PrinterPrintFile>> getFileList(const std::string& printerId) override;
    virtual PrinterNetworkResult<std::vector<PrinterPrintTask>> getPrintTaskList(const std::string& printerId) override;
    virtual PrinterNetworkResult<bool> deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds) override;



    virtual PrinterNetworkResult<std::string> hasInstalledPlugin() override;
    virtual PrinterNetworkResult<bool> installPlugin(const std::string& pluginPath) override;
    virtual PrinterNetworkResult<bool> uninstallPlugin() override;
    virtual PrinterNetworkResult<bool> loginWAN(const NetworkUserInfo& userInfo) override;
    virtual PrinterNetworkResult<std::string> getRtcToken() override;
    virtual PrinterNetworkResult<bool> sendRtmMessage(const std::string& message) override;

    static void init();
    static void uninit();
};

} // namespace Slic3r

#endif
