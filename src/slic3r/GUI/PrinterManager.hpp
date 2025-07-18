#pragma once
#include <map>
#include "slic3r/GUI/PrinterWebView.hpp"
#include "slic3r/GUI/TabButton.hpp"
#include <wx/webview.h>
#include <wx/aui/aui.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "Plater.hpp"
#include "PrintHostDialogs.hpp"
#if wxUSE_WEBVIEW_IE
#include "wx/msw/webview_ie.h"
#endif
#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif

namespace Slic3r { namespace GUI {


enum DeviceStatus {
    DEVICE_STATUS_UNKNOWN = -1,
    DEVICE_STATUS_OFFLINE = 0,
    DEVICE_STATUS_IDLE = 1,
    DEVICE_STATUS_PRINTING = 2,
    DEVICE_STATUS_HOMING = 3,
    DEVICE_STATUS_PAUSED = 4,
    DEVICE_STATUS_STOPPED = 5,
    DEVICE_STATUS_COMPLETED = 6,
    DEVICE_STATUS_RESUMING = 7,
    DEVICE_STATUS_AUTO_LEVELING = 8,
    DEVICE_STATUS_PREHEATING = 9,
    DEVICE_STATUS_ERROR = 999,
};


struct PrinterInfo {
    // machine info
    std::string id;
    std::string name;
    std::string ip;
    std::string port;
    std::string vendor;
    std::string machineName;
    std::string machineModel;
    std::string protocolVersion;
    std::string firmwareVersion;
    std::string deviceId;
    std::string deviceType;
    std::string serialNumber;
    std::string webUrl;
    std::string connectionUrl;
    bool isPhysicalPrinter;

    template<class Archive>
    void serialize(Archive & ar) {
        ar(CEREAL_NVP(id),
           CEREAL_NVP(name),
           CEREAL_NVP(ip),
           CEREAL_NVP(port),
           CEREAL_NVP(vendor),
           CEREAL_NVP(machineName),
           CEREAL_NVP(machineModel),
           CEREAL_NVP(protocolVersion),
           CEREAL_NVP(firmwareVersion),
           CEREAL_NVP(deviceId),
           CEREAL_NVP(deviceType),
           CEREAL_NVP(serialNumber),
           CEREAL_NVP(webUrl),
           CEREAL_NVP(connectionUrl),
           CEREAL_NVP(isPhysicalPrinter));
    }
};

class PrinterManager : public wxPanel
{
public:
    PrinterManager(wxWindow *parent);
    virtual ~PrinterManager();
    void onClose(wxCloseEvent& evt);
private:
    void onScriptMessage(wxWebViewEvent &evt);
    void runScript(const wxString &javascript);
    void notifyPrintInfo(PrinterInfo printerInfo, int printerStatus, int printProgress, int currentTicks, int totalTicks);
    void onClosePrinterTab(wxAuiNotebookEvent& event);
    void openPrinterTab(const std::string& id);
    void loadPrinterList();
    void savePrinterList();
    nlohmann::json getPrinterList();
    nlohmann::json discoverPrinter();
    nlohmann::json bindPrinters(nlohmann::json& printers);
    bool updatePrinterName(const std::string& id, const std::string& name);
    bool deletePrinter(const std::string& id);
    wxAuiNotebook* mTabBar;
    wxWebView* mBrowser;

    std::map<std::string, PrinterInfo> mPrinterList;
    std::map<std::string, PrinterWebView*> mPrinterViews;
   
};
}} // namespace Slic3r::GUI 
