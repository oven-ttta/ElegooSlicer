#pragma once
#include <map>
#include <functional>
#include <atomic>
#include <memory>
#include "slic3r/GUI/PrinterWebView.hpp"
#include <wx/webview.h>
#include <wx/aui/aui.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "Plater.hpp"
#if wxUSE_WEBVIEW_IE
#include "wx/msw/webview_ie.h"
#endif
#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif

namespace Slic3r { namespace GUI {

class PrinterManagerView : public wxPanel
{
public:
    PrinterManagerView(wxWindow *parent);
    virtual ~PrinterManagerView();
    void onClose(wxCloseEvent& evt);
    void openPrinterTab(const std::string& printerId);

private:
    void setupIPCHandlers();
    void runScript(const wxString &javascript);
    void onClosePrinterTab(wxAuiNotebookEvent& event);
 

    nlohmann::json getPrinterList();
    nlohmann::json getPrinterListStatus();
    nlohmann::json discoverPrinter();
    nlohmann::json getPrinterModelList();
    std::string addPrinter(const nlohmann::json& printer);
    std::string addPhysicalPrinter(const nlohmann::json& printer);
    bool updatePrinterName(const std::string& printerId, const std::string& printerName);
    bool updatePrinterHost(const std::string& printerId, const std::string& host);
    bool deletePrinter(const std::string& printerId);
    std::string browseCAFile();

    wxAuiNotebook* mTabBar;
    wxWebView* mBrowser;
    webviewIpc::WebviewIPCManager* m_ipc;
    std::map<std::string, PrinterWebView*> mPrinterViews;
    std::atomic<bool> m_isDestroying;
    std::shared_ptr<bool> m_lifeTracker;
   
};
}} // namespace Slic3r::GUI 
