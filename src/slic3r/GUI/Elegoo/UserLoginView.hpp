#ifndef USERLOGINVIEW_HPP
#define USERLOGINVIEW_HPP

#include <wx/wx.h>
#include <wx/webview.h>
#include <memory>
#include <atomic>
#include "slic3r/Utils/WebviewIPCManager.h"
#include "slic3r/GUI/MsgDialog.hpp"

namespace Slic3r { namespace GUI {

class UserLoginView : public GUI::MsgDialog
{
public:
    UserLoginView(wxWindow *parent);
    ~UserLoginView();
    
    static void ShowLoginDialog();

private:
    void initUI();
    void setupIPCHandlers();
    void cleanupIPC();
    
    // Event handlers
    void onWebViewLoaded(wxWebViewEvent& event);
    void onWebViewError(wxWebViewEvent& event);
    void onClose(wxCloseEvent& event);

private:
    wxWebView* mBrowser;
    std::unique_ptr<webviewIpc::WebviewIPCManager> mIpc;
    
    std::string mLanguage;
    std::string mRegion;
    
    static std::atomic<bool> s_isShown;
    
    DECLARE_EVENT_TABLE()
};

}} // namespace Slic3r::GUI

#endif // USERLOGINVIEW_HPP
