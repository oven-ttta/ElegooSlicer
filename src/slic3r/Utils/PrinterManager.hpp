#pragma once
#include <map>
#include "slic3r/Utils/PrinterNetwork.hpp"
#include "slic3r/Utils/Singleton.hpp"

namespace Slic3r { 

// Multi-device management
// Support multiple device types, but need to adapt network communication, implement PrinterNetwork class
// Network layer supports 1-to-1, many-to-1

class PrinterManager : public Singleton<PrinterManager>
{
    friend class Singleton<PrinterManager>;
public:
    ~PrinterManager();
    PrinterManager(const PrinterManager&) = delete;
    PrinterManager& operator=(const PrinterManager&) = delete;

    std::vector<PrinterNetworkInfo> getPrinterList();
    bool upload(PrinterNetworkParams& params);
    std::vector<PrinterNetworkInfo> discoverPrinter();
    std::string addPrinter(PrinterNetworkInfo& printerNetworkInfo);
    bool updatePrinterName(const std::string& printerId, const std::string& name);
    bool updatePrinterHost(const std::string& printerId, const std::string& host);
    bool deletePrinter(const std::string& printerId);

    static PrinterNetworkInfo convertJsonToPrinterNetworkInfo(const nlohmann::json& json);
    static nlohmann::json convertPrinterNetworkInfoToJson(const PrinterNetworkInfo& printerNetworkInfo);
    static std::map<std::string, std::map<std::string, DynamicPrintConfig>> getVendorPrinterModelConfig();
    static std::string imageFileToBase64DataURI(const std::string& image_path);

    void close();

private:
    PrinterManager();

    void savePrinterList();

    void updatePrinterConnectStatus(const std::string& printerId, const PrinterConnectStatus& status);
    void updatePrinterStatus(const std::string& printerId, const PrinterStatus& status);
    void updatePrinterPrintTask(const std::string& printerId, const PrinterPrintTask& task);
    void updatePrinterAttributes(const std::string& printerId, const PrinterNetworkInfo& printerInfo);

    std::map<std::string, PrinterNetworkInfo> mPrinterList;
    std::mutex mPrinterListMutex;

};
} // namespace Slic3r::GUI 
