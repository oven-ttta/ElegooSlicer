#pragma once
#include <map>
#include <functional>
#include <wx/webview.h>
#include <wx/aui/aui.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "MsgDialog.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#if wxUSE_WEBVIEW_IE
#include "wx/msw/webview_ie.h"
#endif
#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif


namespace Slic3r { namespace GUI {

class PrinterMmsSyncView : public GUI::MsgDialog
{
public:
    PrinterMmsSyncView(wxWindow *parent);
    virtual ~PrinterMmsSyncView();
    virtual int ShowModal() override;
    virtual void EndModal(int ret) override;
    
    PrinterMmsGroup getSyncedMmsGroup();

private:
    void onScriptMessage(wxWebViewEvent &evt);
    void handleCommand(const std::string& method, const std::string& id, const nlohmann::json& params);
    void sendResponse(const std::string& method, const std::string& id, const nlohmann::json& response);
    nlohmann::json getPrinterList();
    nlohmann::json getPrinterFilamentInfo(const nlohmann::json& params);
    nlohmann::json syncMmsFilament(const nlohmann::json& params);
    void runScript(const wxString &javascript);
    void onShow();

    wxWebView* mBrowser;   
    PrinterMmsGroup mMmsGroup;
};
}} // namespace Slic3r::GUI 
