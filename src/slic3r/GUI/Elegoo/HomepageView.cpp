#include "HomepageView.hpp"
#include <wx/webview.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <slic3r/Utils/WebviewIPCManager.h>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/MainFrame.hpp>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <slic3r/GUI/Elegoo/UserLoginView.hpp>
#include <slic3r/Utils/Elegoo/UserNetworkManager.hpp>
#include <slic3r/Utils/Elegoo/ElegooNetworkHelper.hpp>
#include "slic3r/Utils/JsonUtils.hpp"
namespace Slic3r { namespace GUI {

// ============================================================================
// HomepageView Base Class Implementation
// ============================================================================

HomepageView::HomepageView(wxWindow* parent, const wxString& name)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), mName(name)
{}

// ============================================================================
// RecentHomepageView Implementation
// ============================================================================

wxBEGIN_EVENT_TABLE(RecentHomepageView, HomepageView) EVT_WEBVIEW_LOADED(wxID_ANY, RecentHomepageView::onWebViewLoaded)
    EVT_WEBVIEW_ERROR(wxID_ANY, RecentHomepageView::onWebViewError) wxEND_EVENT_TABLE()

        RecentHomepageView::RecentHomepageView(wxWindow* parent)
    : HomepageView(parent, "recent"), mBrowser(nullptr), mIpc(nullptr)
{
    initUI();
}

RecentHomepageView::~RecentHomepageView() { cleanupIPC(); }

void RecentHomepageView::initUI()
{
    // Create webview
    mBrowser = WebView::CreateWebView(this, "");

    if (!mBrowser) {
        wxMessageBox("Failed to create webview", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Remove WebView border completely
    mBrowser->SetWindowStyleFlag(wxNO_BORDER);

    // Set up layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(mBrowser, 1, wxEXPAND, 0);
    SetSizer(sizer);

    // Initialize IPC
    mIpc = std::make_unique<webviewIpc::WebviewIPCManager>(mBrowser);

    setupIPCHandlers();

    // Load recent files page
    wxString url = wxString::Format("file://%s/web/homepage4/recent.html", from_u8(resources_dir()));
    mBrowser->LoadURL(url);

    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
}

void RecentHomepageView::initialize()
{
    // Additional initialization if needed
}

void RecentHomepageView::updateMode()
{
    // Update mode if needed
}
void RecentHomepageView::setupIPCHandlers()
{
    if (!mIpc)
        return;

    mIpc->onRequest("getRecentFiles", [this](const webviewIpc::IPCRequest& request) { return handleGetRecentFiles(request.params); });

    mIpc->onRequest("clearRecentFiles", [this](const webviewIpc::IPCRequest& request) { return handleClearRecentFiles(request.params); });

    mIpc->onRequest("openFile", [this](const webviewIpc::IPCRequest& request) { return handleOpenFile(request.params); });

    mIpc->onRequest("createNewProject", [this](const webviewIpc::IPCRequest& request) { return handleCreateNewProject(request.params); });

    mIpc->onRequest("openProject", [this](const webviewIpc::IPCRequest& request) { return handleOpenProject(request.params); });

    mIpc->onRequest("openFileInExplorer",
                    [this](const webviewIpc::IPCRequest& request) { return handleOpenFileInExplorer(request.params); });

    mIpc->onRequest("removeFromRecent", [this](const webviewIpc::IPCRequest& request) { return handleRemoveFromRecent(request.params); });
}

void RecentHomepageView::cleanupIPC() {}

webviewIpc::IPCResult RecentHomepageView::handleGetRecentFiles(const nlohmann::json& data)
{
    // TODO: Implement recent files loading
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult RecentHomepageView::handleClearRecentFiles(const nlohmann::json& data)
{
    wxGetApp().request_remove_project("");
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult RecentHomepageView::handleOpenFile(const nlohmann::json& data)
{
    std::string filePath = data.value("path", "");
    if (!filePath.empty()) {
        wxGetApp().request_open_project(filePath);
    }
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult RecentHomepageView::handleCreateNewProject(const nlohmann::json& data)
{
    wxGetApp().request_open_project("<new>");
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult RecentHomepageView::handleOpenProject(const nlohmann::json& data)
{
    wxGetApp().request_open_project({});
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult RecentHomepageView::handleOpenFileInExplorer(const nlohmann::json& data)
{
    std::string filePath = data.value("path", "");
    if (!filePath.empty()) {
        boost::filesystem::path file_path(filePath);
        std::string             file_path_str = file_path.make_preferred().string();
        wxLaunchDefaultBrowser("file://" + file_path_str);
    }
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult RecentHomepageView::handleRemoveFromRecent(const nlohmann::json& data)
{
    std::string filePath = data.value("path", "");
    if (!filePath.empty()) {
        wxGetApp().request_remove_project(filePath);
    }
    return webviewIpc::IPCResult::success();
}

void RecentHomepageView::onWebViewLoaded(wxWebViewEvent& event)
{
    // IPC is already initialized in constructor
    mIsReady = true;
    wxLogMessage("RecentHomepageView: WebView loaded successfully");
}

void RecentHomepageView::onWebViewError(wxWebViewEvent& event)
{
    wxString error = event.GetString();
    wxMessageBox("WebView Error: " + error, "Error", wxOK | wxICON_ERROR);
}

// ============================================================================
// OnlineModelsHomepageView Implementation
// ============================================================================

wxBEGIN_EVENT_TABLE(OnlineModelsHomepageView, HomepageView) EVT_WEBVIEW_LOADED(wxID_ANY, OnlineModelsHomepageView::onWebViewLoaded)
    EVT_WEBVIEW_ERROR(wxID_ANY, OnlineModelsHomepageView::onWebViewError) wxEND_EVENT_TABLE()

        OnlineModelsHomepageView::OnlineModelsHomepageView(wxWindow* parent)
    : HomepageView(parent, "online-models"), mBrowser(nullptr), mIpc(nullptr)
{
    initUI();
}

OnlineModelsHomepageView::~OnlineModelsHomepageView() { cleanupIPC(); }

void OnlineModelsHomepageView::initUI()
{
    // Create webview
    mBrowser = WebView::CreateWebView(this, "");

    if (!mBrowser) {
        wxMessageBox("Failed to create webview", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Remove WebView border completely
    mBrowser->SetWindowStyleFlag(wxNO_BORDER);

    // Set up layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(mBrowser, 1, wxEXPAND, 0);
    SetSizer(sizer);

    // Initialize IPC
    mIpc = std::make_unique<webviewIpc::WebviewIPCManager>(mBrowser);

    setupIPCHandlers();

    // Load online models page from remote URL
    std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHostType::htElegooLink);
    if (!networkHelper) {
        wxLogError("Could not create network helper");
        return;
    }
    
    std::string region = networkHelper->getRegion();
    if (region == "CN") {
        wxLogMessage("China region detected, online models page will not be loaded");
        return;
    }

    std::string url = networkHelper->getOnlineModelsUrl();
    mBrowser->SetUserAgent(networkHelper->getUserAgent());
    mBrowser->LoadURL(url);
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
}

void OnlineModelsHomepageView::initialize()
{
    // Additional initialization if needed
}

void OnlineModelsHomepageView::updateMode()
{
    // Update mode if needed
}

nlohmann::json generateUserInfoData(const UserNetworkInfo& userNetworkInfo)
{
    nlohmann::json data;
    data["userId"]       = userNetworkInfo.userId;
    data["accessToken"]  = userNetworkInfo.token;
    data["refreshToken"] = userNetworkInfo.refreshToken;
    data["expiresTime"]  = userNetworkInfo.accessTokenExpireTime;
    data["refreshTokenExpireTime"] = userNetworkInfo.refreshTokenExpireTime;
    data["avatar"] = userNetworkInfo.avatar;
    data["email"] = userNetworkInfo.email;
    data["nickname"] = userNetworkInfo.nickname;
    data["loginStatus"]  = userNetworkInfo.loginStatus;
    data["lastTokenRefreshTime"] = userNetworkInfo.lastTokenRefreshTime;
    return data;
}

UserNetworkInfo parseUserInfoData(const nlohmann::json& data)
{
    UserNetworkInfo userNetworkInfo;
    userNetworkInfo.userId = JsonUtils::safeGetString(data, "userId", "");
    userNetworkInfo.refreshToken = JsonUtils::safeGetString(data, "refreshToken", "");
    userNetworkInfo.token = JsonUtils::safeGetString(data, "accessToken", "");
    userNetworkInfo.accessTokenExpireTime = JsonUtils::safeGetInt64(data, "expiresTime", 0);
    userNetworkInfo.refreshTokenExpireTime = JsonUtils::safeGetInt64(data, "refreshTokenExpireTime", 0);
    userNetworkInfo.lastTokenRefreshTime = JsonUtils::safeGetInt64(data, "lastTokenRefreshTime", 0);
    userNetworkInfo.loginStatus = LoginStatus(JsonUtils::safeGetInt(data, "loginStatus", static_cast<int>(LOGIN_STATUS_NO_USER)));
    userNetworkInfo.avatar = JsonUtils::safeGetString(data, "avatar", "");
    userNetworkInfo.email = JsonUtils::safeGetString(data, "email", "");
    userNetworkInfo.nickname = JsonUtils::safeGetString(data, "nickname", "");
    return userNetworkInfo;
}
void OnlineModelsHomepageView::setupIPCHandlers()
{
    if (!mIpc)
        return;

    mIpc->onRequest("report.getClientUserInfo", [this](const webviewIpc::IPCRequest& request) {
        UserNetworkInfo userNetworkInfo = UserNetworkManager::getInstance()->getUserInfo();
        if (userNetworkInfo.userId.empty() || 
            userNetworkInfo.token.empty() ||
            userNetworkInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN ||
            userNetworkInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER) {
            return webviewIpc::IPCResult::error();
        }
        nlohmann::json  data = generateUserInfoData(userNetworkInfo);
        return webviewIpc::IPCResult::success(data);
        
    });

    mIpc->onRequest("report.notLogged", [this](const webviewIpc::IPCRequest& request) {
        UserNetworkInfo userNetworkInfo = UserNetworkManager::getInstance()->getUserInfo();
        auto result = UserNetworkManager::getInstance()->checkUserNeedReLogin();
        if(result.isSuccess()) {
            bool needReLogin = result.data.value();
            if(needReLogin) {
                //need re-login
                auto evt = new wxCommandEvent(EVT_USER_LOGIN);
                wxQueueEvent(wxGetApp().mainframe, evt);
                return webviewIpc::IPCResult::error();
            } 
        } else {
            //show_error(wxGetApp().mainframe, result.message);
            return webviewIpc::IPCResult::error(result.message);
        }
        //don't need to re-login, return user info
        nlohmann::json  data = generateUserInfoData(userNetworkInfo);
        return webviewIpc::IPCResult::success(data);
    });

    mIpc->onRequest("report.refreshToken", [this](const webviewIpc::IPCRequest& request) { 
        auto        data     = request.params;
        UserNetworkInfo userNetworkInfo = parseUserInfoData(data);
        bool result = UserNetworkManager::getInstance()->refreshToken(userNetworkInfo);
        userNetworkInfo = UserNetworkManager::getInstance()->getUserInfo();
        nlohmann::json userData = generateUserInfoData(userNetworkInfo);
        if(result) {
            return webviewIpc::IPCResult::success(userData);
        } 
        return webviewIpc::IPCResult::error(userData);
    });
    mIpc->onRequest("report.ready", [this](const webviewIpc::IPCRequest& request) { return handleReady(); });

    mIpc->onRequest("report.slicerOpen", [this](const webviewIpc::IPCRequest& request) {
        auto        params = request.params;
        std::string url    = params.value("url", "");

        GUI::wxGetApp().request_model_download(wxString(url));

        return webviewIpc::IPCResult::success();
    });
    mIpc->onRequest("report.websiteOpen", [this](const webviewIpc::IPCRequest& request) {
        auto        params = request.params;
        std::string url    = params.value("url", "");
        wxLaunchDefaultBrowser(url);
        return webviewIpc::IPCResult::success();
    });
}
void OnlineModelsHomepageView::onUserInfoUpdated(const UserNetworkInfo& userNetworkInfo)
{
    lock_guard<mutex> lock(mUserInfoMutex);
    // Update user info in webview
    if (!mIpc || !mIsReady) {
        mRefreshUserInfo = userNetworkInfo;
        return;
    }
    mRefreshUserInfo = UserNetworkInfo();
    nlohmann::json data = generateUserInfoData(userNetworkInfo);
    mIpc->sendEvent("client.onUserInfoUpdated", data, mIpc->generateRequestId());
    wxLogMessage("OnlineModelsHomepageView: Sent user info to WebView");
}

void OnlineModelsHomepageView::onRegionChanged()
{
    //reload online models page
    initUI();
}

void OnlineModelsHomepageView::cleanupIPC()
{

}

webviewIpc::IPCResult OnlineModelsHomepageView::handleReady()
{
    lock_guard<mutex> lock(mUserInfoMutex);
    mIsReady = true;
    if (mIpc && !mRefreshUserInfo.userId.empty()) {
        nlohmann::json data = generateUserInfoData(mRefreshUserInfo);
        mIpc->sendEvent("client.onUserInfoUpdated", data, mIpc->generateRequestId());
        mRefreshUserInfo = UserNetworkInfo();
        wxLogMessage("OnlineModelsHomepageView: Sent user info to WebView");
    }
    return webviewIpc::IPCResult::success();
}
void OnlineModelsHomepageView::onWebViewLoaded(wxWebViewEvent& event) {}

void OnlineModelsHomepageView::onWebViewError(wxWebViewEvent& event)
{
    wxString error = event.GetString();
    // wxMessageBox("WebView Error: " + error, "Error", wxOK | wxICON_ERROR);
}

}} // namespace Slic3r::GUI
