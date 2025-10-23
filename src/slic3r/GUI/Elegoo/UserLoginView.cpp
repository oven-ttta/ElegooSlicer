#include "UserLoginView.hpp"
#include <wx/webview.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <slic3r/Utils/WebviewIPCManager.h>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include "slic3r/GUI/I18N.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include <boost/filesystem.hpp>
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/Utils/JsonUtils.hpp"
#include "slic3r/Utils/Elegoo/UserNetworkManager.hpp"
#include "slic3r/Utils/Elegoo/ElegooNetworkHelper.hpp"
namespace Slic3r { namespace GUI {

std::atomic<bool> UserLoginView::s_isShown{false};

wxBEGIN_EVENT_TABLE(UserLoginView, MsgDialog) EVT_CLOSE(UserLoginView::onClose) wxEND_EVENT_TABLE()

void UserLoginView::ShowLoginDialog()
{
    bool expected = false;
    if (!s_isShown.compare_exchange_strong(expected, true)) {
        wxLogMessage("UserLoginView already shown, ignore duplicate request");
        return;
    }
    
    UserLoginView* dialog = new UserLoginView(wxGetApp().mainframe);
    dialog->ShowModal();
    delete dialog;
}

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

UserLoginView::~UserLoginView()
{
    s_isShown = false;
    cleanupIPC();
}

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
    std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHostType::htElegooLink);
    if (!networkHelper) {
        wxLogError("Could not create network helper");
        return;
    } 
    std::string url = networkHelper->getLoginUrl();
    mLanguage = networkHelper->getLanguage();
    mRegion = networkHelper->getRegion();
    mBrowser->SetUserAgent(networkHelper->getUserAgent());
    mBrowser->LoadURL(url);
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
        std::string     xxx  = data.dump();
        UserNetworkInfo userNetworkInfo;
        userNetworkInfo.userId = JsonUtils::safeGetString(data, "userId", "");
        userNetworkInfo.token = JsonUtils::safeGetString(data, "accessToken", "");
        userNetworkInfo.accessTokenExpireTime = JsonUtils::safeGetInt64(data, "accessTokenExpireTime", 0);
        userNetworkInfo.refreshToken = JsonUtils::safeGetString(data, "refreshToken", "");
        userNetworkInfo.refreshTokenExpireTime = JsonUtils::safeGetInt64(data, "refreshTokenExpireTime", 0);
        userNetworkInfo.openid = JsonUtils::safeGetString(data, "openid", "");
        userNetworkInfo.avatar = JsonUtils::safeGetString(data, "avatar", "");
        userNetworkInfo.email = JsonUtils::safeGetString(data, "email", "");
        userNetworkInfo.nickname = JsonUtils::safeGetString(data, "nickname", "");
        userNetworkInfo.hostType = PrintHost::get_print_host_type_str(PrintHostType::htElegooLink);
        userNetworkInfo.phone = JsonUtils::safeGetString(data, "phone", "");

        std::string bakId = std::to_string(JsonUtils::safeGetInt64(data, "bakId", -1));
        std::string avatarPath = JsonUtils::safeGetString(data, "avatarPath", "");
        nlohmann::json extraInfoJson=nlohmann::json::object();
        extraInfoJson["bakId"] = bakId;
        extraInfoJson["avatarPath"] = avatarPath;
        userNetworkInfo.extraInfo = extraInfoJson.dump();

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
