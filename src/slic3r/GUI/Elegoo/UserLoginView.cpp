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
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
namespace Slic3r { namespace GUI {

std::atomic<bool> UserLoginView::s_isShown{false};

wxBEGIN_EVENT_TABLE(UserLoginView, MsgDialog) EVT_CLOSE(UserLoginView::onClose) wxEND_EVENT_TABLE()

void UserLoginView::ShowLoginDialog()
{
    bool expected = false;
    if (!s_isShown.compare_exchange_strong(expected, true)) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": UserLoginView already shown, ignore duplicate request";
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
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": could not init m_browser";
        return;
    }

    mIpc = std::make_unique<webviewIpc::WebviewIPCManager>(mBrowser);
    setupIPCHandlers();

    // Load login page - online URL
    std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHostType::htElegooLink);
    if (!networkHelper) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": could not create network helper";
        return;
    } 
    std::string url = networkHelper->getLoginUrl();
    mLanguage = networkHelper->getLanguage();
    mRegion = networkHelper->getRegion();
    mBrowser->SetUserAgent(networkHelper->getUserAgent());
    mBrowser->LoadURL(url);
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
    mBrowser->Bind(wxEVT_WEBVIEW_ERROR, &UserLoginView::onWebViewError, this);
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    topsizer->Add(mBrowser, wxSizerFlags().Expand().Proportion(1));
    wxSize pSize = FromDIP(wxSize(496, 723));
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

        auto now = std::chrono::system_clock::now();
        userNetworkInfo.lastTokenRefreshTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        userNetworkInfo.loginTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        if(!userNetworkInfo.userId.empty() && !userNetworkInfo.token.empty()) {
            userNetworkInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;
        } else {
            return webviewIpc::IPCResult::error();
        }
   
        UserNetworkManager::getInstance()->login(userNetworkInfo);

        CallAfter([this]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            EndModal(wxID_OK);
        });
      
        return webviewIpc::IPCResult::success();
    });

    mIpc->onRequest("report.websiteOpen", [this](const webviewIpc::IPCRequest& request) {
        auto        params = request.params;
        std::string url    = params.value("url", "");
        CallAfter([this, url]() {
            wxLaunchDefaultBrowser(url);
        });
        return webviewIpc::IPCResult::success();
    });

    mIpc->onRequest("reload", [this](const webviewIpc::IPCRequest& request) {
        if(mIsLoading){
          return webviewIpc::IPCResult::success();
        }
        std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHostType::htElegooLink);
        if (networkHelper) {
            std::string url = networkHelper->getLoginUrl();
            CallAfter([this, url]() {
                mBrowser->LoadURL(url);
            });
        }
        return webviewIpc::IPCResult::success();
    });
}

void UserLoginView::cleanupIPC()
{
}

void UserLoginView::onWebViewLoaded(wxWebViewEvent& event) {
    mIsLoading = false;
}
void UserLoginView::OnNavigationRequest(wxWebViewEvent& evt){
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << ": " << evt.GetTarget().ToUTF8().data();
    mIsLoading = true;
}
void UserLoginView::OnNavigationComplete(wxWebViewEvent& evt){
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << ": " << evt.GetTarget().ToUTF8().data();
    mIsLoading = false;
}

#define FAILED_URL_SUFFIX "/web/error-page/connection-failed.html"
void UserLoginView::onWebViewError(wxWebViewEvent& evt)
{
    auto e = "unknown error";
    switch (evt.GetInt()) {
    case wxWEBVIEW_NAV_ERR_CONNECTION: e = "wxWEBVIEW_NAV_ERR_CONNECTION"; break;
    case wxWEBVIEW_NAV_ERR_CERTIFICATE: e = "wxWEBVIEW_NAV_ERR_CERTIFICATE"; break;
    case wxWEBVIEW_NAV_ERR_AUTH: e = "wxWEBVIEW_NAV_ERR_AUTH"; break;
    case wxWEBVIEW_NAV_ERR_SECURITY: e = "wxWEBVIEW_NAV_ERR_SECURITY"; break;
    case wxWEBVIEW_NAV_ERR_NOT_FOUND: e = "wxWEBVIEW_NAV_ERR_NOT_FOUND"; break;
    case wxWEBVIEW_NAV_ERR_REQUEST: e = "wxWEBVIEW_NAV_ERR_REQUEST"; break;
    case wxWEBVIEW_NAV_ERR_USER_CANCELLED: e = "wxWEBVIEW_NAV_ERR_USER_CANCELLED"; break;
    case wxWEBVIEW_NAV_ERR_OTHER: e = "wxWEBVIEW_NAV_ERR_OTHER"; break;
    }
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__
                            << boost::format(": error loading page %1% %2% %3% %4%") % evt.GetURL() % evt.GetTarget() % e % evt.GetString();

    std::string url     = evt.GetURL().ToStdString();
    std::string target  = evt.GetTarget().ToStdString();
    std::string error   = e;
    std::string message = evt.GetString().ToStdString();
    mIsLoading = false;
    if(url.find(FAILED_URL_SUFFIX)!=std::string::npos){
        return;
    }


    auto code = evt.GetInt();
    if (code == wxWEBVIEW_NAV_ERR_CONNECTION || code == wxWEBVIEW_NAV_ERR_NOT_FOUND || code == wxWEBVIEW_NAV_ERR_REQUEST) {
        std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                wxGetApp().CallAfter([this]() {
                    auto path = resources_dir() + FAILED_URL_SUFFIX;
                    #if WIN32
                        std::replace(path.begin(), path.end(), '/', '\\');
                    #endif
                    auto failedUrl = wxString::Format("file:///%s", from_u8(path));
                    mBrowser->LoadURL(failedUrl);
                });
        }).detach();
    }
}

void UserLoginView::onClose(wxCloseEvent& event)
{
    cleanupIPC();
    event.Skip();
}

}} // namespace Slic3r::GUI
