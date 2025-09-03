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

#include "slic3r/Utils/WebviewIPCManager.h"
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
    void onClosePrinterTab(wxAuiNotebookEvent& event);
    void onTabBeginDrag(wxAuiNotebookEvent& event);
    void onTabDragMotion(wxAuiNotebookEvent& event);
    void onTabEndDrag(wxAuiNotebookEvent& event);
    bool mFirstTabClicked{false};

    webviewIpc::IPCResult getPrinterList();
    webviewIpc::IPCResult getPrinterListStatus();
    webviewIpc::IPCResult discoverPrinter();
    webviewIpc::IPCResult getPrinterModelList();
    webviewIpc::IPCResult addPrinter(const nlohmann::json& printer);
    webviewIpc::IPCResult addPhysicalPrinter(const nlohmann::json& printer);
    webviewIpc::IPCResult updatePrinterName(const std::string& printerId, const std::string& printerName);
    webviewIpc::IPCResult updatePrinterHost(const std::string& printerId, const std::string& host);
    webviewIpc::IPCResult deletePrinter(const std::string& printerId);
    webviewIpc::IPCResult browseCAFile();

    wxAuiNotebook* mTabBar;
    wxWebView* mBrowser;
    webviewIpc::WebviewIPCManager* m_ipc;
    std::map<std::string, PrinterWebView*> mPrinterViews;
    std::atomic<bool> m_isDestroying;
    std::shared_ptr<bool> m_lifeTracker;
   
};
}} // namespace Slic3r::GUI 
