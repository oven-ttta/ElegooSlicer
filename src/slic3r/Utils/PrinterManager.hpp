#pragma once
#include <map>
#include "slic3r/Utils/PrinterNetwork.hpp"
#include "slic3r/Utils/Singleton.hpp"

namespace Slic3r { 

class PrinterManager : public Singleton<PrinterManager>
{
    friend class Singleton<PrinterManager>;
public:
    virtual ~PrinterManager();
    PrinterManager::PrinterManager(const PrinterManager&) = delete;
    PrinterManager& PrinterManager::operator=(const PrinterManager&) = delete;

public:
    std::vector<PrinterNetworkInfo> getPrinterList();
    bool upload(PrinterNetworkParams& params);
    std::vector<PrinterNetworkInfo> discoverPrinter();
    std::string bindPrinter(PrinterNetworkInfo& printerNetworkInfo);
    bool updatePrinterName(const std::string& id, const std::string& name);
    bool deletePrinter(const std::string& id);

private:
    PrinterManager();
    void loadPrinterList();
    void savePrinterList();

    std::map<std::string, PrinterNetworkInfo> mPrinterList;

};
} // namespace Slic3r::GUI 
