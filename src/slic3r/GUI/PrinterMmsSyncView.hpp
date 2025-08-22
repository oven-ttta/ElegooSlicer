#pragma once
#include <map>
#include <functional>
#include <memory>
#include <atomic>
#include <wx/webview.h>
#include <wx/aui/aui.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "MsgDialog.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include <slic3r/Utils/WebviewIPCManager.h>
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
    void setupIPCHandlers();
    void onScriptMessage(wxWebViewEvent &evt);
    void OnCloseWindow(wxCloseEvent& event);
    nlohmann::json getPrinterList();
    nlohmann::json getPrinterFilamentInfo(const nlohmann::json& params);
    nlohmann::json syncMmsFilament(const nlohmann::json& params);
    void runScript(const wxString &javascript);
    void onShow();

    wxWebView* mBrowser;   
    webviewIpc::WebviewIPCManager* mIpc;
    PrinterMmsGroup mMmsGroup;
    std::atomic<bool> m_isDestroying;
    std::shared_ptr<bool> m_lifeTracker;
    std::atomic<bool> m_asyncOperationInProgress;
};
}} // namespace Slic3r::GUI 
