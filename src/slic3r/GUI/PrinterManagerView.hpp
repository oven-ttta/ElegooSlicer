#pragma once
#include <map>
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


private:
    void onScriptMessage(wxWebViewEvent &evt);
    void runScript(const wxString &javascript);
    void onClosePrinterTab(wxAuiNotebookEvent& event);
    void openPrinterTab(const std::string& id);

    nlohmann::json getPrinterList();
    nlohmann::json discoverPrinter();
    nlohmann::json getPrinterModelList();
    std::string bindPrinter(const nlohmann::json& printer);
    bool updatePrinterName(const std::string& id, const std::string& name);
    bool deletePrinter(const std::string& id);
    std::string browseCAFile();

    wxAuiNotebook* mTabBar;
    wxWebView* mBrowser;
    std::map<std::string, PrinterWebView*> mPrinterViews;

};
}} // namespace Slic3r::GUI 
