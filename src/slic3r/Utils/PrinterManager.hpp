#pragma once
#include <map>
#include "slic3r/Utils/PrinterNetwork.hpp"
#include "slic3r/Utils/Singleton.hpp"

namespace Slic3r { 

enum PrinterStatus {
    PRINTER_STATUS_UNKNOWN = -1,
    PRINTER_STATUS_OFFLINE = 0,
    PRINTER_STATUS_IDLE = 1,
    PRINTER_STATUS_PRINTING = 2,
    PRINTER_STATUS_HOMING = 3,
    PRINTER_STATUS_PAUSED = 4,
    PRINTER_STATUS_STOPPED = 5,
    PRINTER_STATUS_COMPLETED = 6,
    PRINTER_STATUS_RESUMING = 7,
    PRINTER_STATUS_AUTO_LEVELING = 8,
    PRINTER_STATUS_PREHEATING = 9,
    PRINTER_STATUS_ERROR = 999,
};

class PrinterManager : public Singleton<PrinterManager>
{
    friend class Singleton<PrinterManager>;
public:
    virtual ~PrinterManager();
    PrinterManager::PrinterManager(const PrinterManager&) = delete;
    PrinterManager& PrinterManager::operator=(const PrinterManager&) = delete;

public:
    std::vector<PrinterInfo> getPrinterList();
    bool upload(PrintHostUpload upload_data, PrintHost::ProgressFn prorgess_fn, PrintHost::ErrorFn error_fn, PrintHost::InfoFn info_fn);
    std::vector<PrinterInfo> discoverPrinter();
    std::string bindPrinter(PrinterInfo& printer);
    bool updatePrinterName(const std::string& id, const std::string& name);
    bool deletePrinter(const std::string& id);
    void notifyPrintInfo(PrinterInfo printerInfo, int printerStatus, int printProgress, int currentTicks, int totalTicks);

private:
    PrinterManager();
    void loadPrinterList();
    void savePrinterList();

    std::map<std::string, PrinterInfo> mPrinterList;

};
} // namespace Slic3r::GUI 
