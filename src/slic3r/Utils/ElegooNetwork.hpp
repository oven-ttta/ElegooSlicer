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
    virtual PrinterNetworkResult<PrinterPrintFileResponse> getFileList(int pageNumber, int pageSize) override;
    virtual PrinterNetworkResult<PrinterPrintTaskResponse> getPrintTaskList(int pageNumber, int pageSize) override;
    virtual PrinterNetworkResult<bool> deletePrintTasks(const std::vector<std::string>& taskIds) override;
    virtual PrinterNetworkResult<bool> sendRtmMessage(const std::string& message) override;
    virtual PrinterNetworkResult<PrinterPrintFileResponse> getFileDetail(const std::string& fileName) override;
    
    virtual PrinterNetworkResult<PrinterNetworkInfo> bindWANPrinter(const PrinterNetworkInfo& printerNetworkInfo) override;
    virtual PrinterNetworkResult<bool> unbindWANPrinter(const std::string& printerId) override;

    virtual void init() override;
    virtual void uninit() override;
};

} // namespace Slic3r

#endif
