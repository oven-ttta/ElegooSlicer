#include "UserLoginView.hpp"
#include <wx/webview.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <slic3r/Utils/WebviewIPCManager.h>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include <boost/filesystem.hpp>
#include "MainFrame.hpp"
#include "slic3r/Utils/PrinterManager.hpp"

namespace Slic3r { namespace GUI {

wxBEGIN_EVENT_TABLE(UserLoginView, MsgDialog) EVT_CLOSE(UserLoginView::onClose) wxEND_EVENT_TABLE()

    UserLoginView::UserLoginView(wxWindow* parent)
    : MsgDialog(static_cast<wxWindow*>(wxGetApp().mainframe), _L("Login"), _L(""), 0)
{
    // Bind close event to handle async operations
    Bind(wxEVT_CLOSE_WINDOW, &UserLoginView::onClose, this);

    // Bind ESC key hook to disable ESC key closing the dialog
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE) {
            // Do nothing - disable ESC key closing the dialog
            return;
        }
        e.Skip();
    });

    SetIcon(wxNullIcon);

    initUI();
}

UserLoginView::~UserLoginView() { cleanupIPC(); }

void UserLoginView::initUI()
{
    // Create webview
    mBrowser = WebView::CreateWebView(this, "");
    if (mBrowser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    // Load login page - online URL
    wxString url = wxString::Format("https://np-sit.elegoo.com.cn/account/slicer-login?");

    if (wxGetApp().app_config->get_language_code() == "zh-cn") {
        url += "language=zh-CN";
    } else {
        url += "language=en";
    }

    if (wxGetApp().app_config->get_country_code() == "CN") {
        url += "&region=CN";
    } else {
        url += "&region=US";
    }

    mBrowser->LoadURL(url);

    mIpc = std::make_unique<webviewIpc::WebviewIPCManager>(mBrowser);
    setupIPCHandlers();
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    topsizer->Add(mBrowser, wxSizerFlags().Expand().Proportion(1));
    wxSize pSize = FromDIP(wxSize(650, 840));
    SetSize(pSize);
    CenterOnParent();

    // Hide the logo if it exists
    if (logo) {
        logo->Hide();
    }
}

void UserLoginView::setupIPCHandlers()
{
    if (!mIpc)
        return;

    mIpc->onRequest("report.userInfo", [this](const webviewIpc::IPCRequest& request) {
        auto        data     = request.params;
        std::string data_str = data.dump();

        UserNetworkInfo userNetworkInfo;

        if (data.contains("userId")) {
            userNetworkInfo.userId = data.value("userId", "");
        }
        if (data.contains("accessToken")) {
            userNetworkInfo.token = data.value("accessToken", "");
        }
        if (data.contains("refreshToken")) {
            userNetworkInfo.refreshToken = data.value("refreshToken", "");
        }
        if (data.contains("expiresTime")) {
            userNetworkInfo.accessTokenExpireTime = data.value("expiresTime", 0);
        }
        if (data.contains("openid")) {
            userNetworkInfo.openid = data.value("openid", "");
        }
        if (data.contains("avatar")) {
            userNetworkInfo.avatar = data.value("avatar", "");
        }
        if (data.contains("email")) {
            userNetworkInfo.email = data.value("email", "");
        }
        if (data.contains("nickname")) {
            userNetworkInfo.nickname = data.value("nickname", "");
        }
        userNetworkInfo.hostType = PrintHost::get_print_host_type_str(PrintHostType::htElegooLink);
        userNetworkInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;

        auto evt = new wxCommandEvent(EVT_USER_INFO_UPDATED);
        wxQueueEvent(wxGetApp().mainframe, evt);

        PrinterManager::getInstance()->setIotUserInfo(userNetworkInfo);
        return webviewIpc::IPCResult::success();
    });

    mIpc->onRequest("report.websiteOpen", [this](const webviewIpc::IPCRequest& request) {
        auto        params = request.params;
        std::string url    = params.value("url", "");
        wxLaunchDefaultBrowser(url);
        return webviewIpc::IPCResult::success();
    });
}

void UserLoginView::cleanupIPC()
{
}
void UserLoginView::onWebViewLoaded(wxWebViewEvent& event)
{
    // WebView loaded successfully
    if (mIpc) {
        mIpc->initialize();
    }
}

void UserLoginView::onWebViewError(wxWebViewEvent& event)
{
    wxString error = event.GetString();
    wxString url   = event.GetURL();

    // Provide more detailed error information
    wxString errorMsg = wxString::Format("WebView Error:\n\nError: %s\nURL: %s\n\nThis might be due to:\n- Network connectivity issues\n- "
                                         "WebView2 runtime problems\n- Invalid URL format",
                                         error, url);

    wxMessageBox(errorMsg, "WebView Error", wxOK | wxICON_ERROR);
}

void UserLoginView::onClose(wxCloseEvent& event)
{
    cleanupIPC();
    event.Skip();
}

}} // namespace Slic3r::GUI
