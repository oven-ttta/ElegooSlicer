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
#include "slic3r/Utils/UserNetworkManager.hpp"
#include "slic3r/Utils/JsonUtils.hpp"
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

    mIpc = std::make_unique<webviewIpc::WebviewIPCManager>(mBrowser);
    setupIPCHandlers();

    // Load login page - online URL
    wxString url = wxString::Format("https://np-sit.elegoo.com.cn/account/slicer-login?");

    std::string language = wxGetApp().app_config->get_language_code();
    language = boost::to_upper_copy(language);
    if (language == "ZH-CN") {
        mLanguage = "zh-CN";
    } else if (language == "ZH-TW") {
        mLanguage = "zh-TW";
    } else if (language == "EN") {
        mLanguage = "en";
    } else if (language == "ES") {
        mLanguage = "es";
    } else if (language == "FR") {
        mLanguage = "fr";
    } else if (language == "DE") {
        mLanguage = "de";
    } else if (language == "JA") {
        mLanguage = "ja";
    } else if (language == "KO") {
        mLanguage = "ko";
    } else if (language == "RU") {
        mLanguage = "ru";
    } else if (language == "PT") {
        mLanguage = "pt";
    } else if (language == "IT") {
        mLanguage = "it";
    } else if (language == "NL") {
        mLanguage = "nl";
    } else if (language == "TR") {
        mLanguage = "tr";
    } else if (language == "CS") {
        mLanguage = "cs";
    } else {
        mLanguage = "en";
    }

    url += "language=" + mLanguage;

    std::string region = wxGetApp().app_config->get_region();
    region = boost::to_upper_copy(region);
    if (region == "CHN" || region == "CHINA") {
        mRegion = "CN";
    } else if (region == "USA" || region == "NORTH AMERICA") {
        mRegion = "US";
    } else if (region == "EUROPE") {
        mRegion = "GB";
    } else if (region == "ASIA-PACIFIC") {
        mRegion = "JP";
    } else {
        mRegion = "other";
    }

    url += "&region=" + mRegion;

    mBrowser->LoadURL(url);

    // 设置 ElegooSlicer UserAgent
    wxString theme = wxGetApp().dark_mode() ? "dark" : "light";
    #ifdef __WIN32__
        mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (Windows NT 10.0; Win64; x64)", SLIC3R_VERSION, theme));
    #elif defined(__WXMAC__)
        mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)", SLIC3R_VERSION, theme));
    #elif defined(__linux__)
        mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (X11; Linux x86_64)", SLIC3R_VERSION, theme));
    #else
        mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (compatible; ElegooSlicer)", SLIC3R_VERSION, theme));
    #endif 
     
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

        UserNetworkInfo userNetworkInfo;
        userNetworkInfo.userId = JsonUtils::safeGetString(data, "userId", "");
        userNetworkInfo.token = JsonUtils::safeGetString(data, "accessToken", "");
        userNetworkInfo.refreshToken = JsonUtils::safeGetString(data, "refreshToken", "");
        userNetworkInfo.accessTokenExpireTime = JsonUtils::safeGetInt(data, "expiresTime", 0);
        userNetworkInfo.openid = JsonUtils::safeGetString(data, "openid", "");
        userNetworkInfo.avatar = JsonUtils::safeGetString(data, "avatar", "");
        userNetworkInfo.email = JsonUtils::safeGetString(data, "email", "");
        userNetworkInfo.nickname = JsonUtils::safeGetString(data, "nickname", "");
        userNetworkInfo.hostType = PrintHost::get_print_host_type_str(PrintHostType::htElegooLink);
        
        userNetworkInfo.region = mRegion;
        userNetworkInfo.language = mLanguage;

        auto now = std::chrono::steady_clock::now();
        userNetworkInfo.lastTokenRefreshTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
        userNetworkInfo.loginTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
        if(!userNetworkInfo.userId.empty() && !userNetworkInfo.token.empty()) {
            userNetworkInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;
        } else {
            return webviewIpc::IPCResult::error();
        }
   
        UserNetworkManager::getInstance()->setIotUserInfo(userNetworkInfo);

        auto evt = new wxCommandEvent(EVT_USER_INFO_UPDATED);
        wxQueueEvent(wxGetApp().mainframe, evt);
      
        CallAfter([this]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            EndModal(wxID_OK);
        });
      
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
