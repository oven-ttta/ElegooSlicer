#include "HomepageView.hpp"
#include <wx/webview.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/url.h>
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
    // SetDropTarget(nullptr);  // Commented out to enable drag-and-drop file support
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
    if (mBrowser) {
        mBrowser->EnableContextMenu(true); 
        mBrowser->SetFocus();
    }
    // Bind(wxEVT_WEBVIEW_NAVIGATING, &RecentHomepageView::OnNavigationRequest, this);
    // Bind(wxEVT_WEBVIEW_NAVIGATED, &RecentHomepageView::OnNavigationComplete, this);
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

    mIpc->onEvent("openRecentFile", [this](const webviewIpc::IPCEvent& event) { return handleOpenFile(event.data); });

    mIpc->onEvent("createNewProject", [this](const webviewIpc::IPCEvent& event) { return handleCreateNewProject(event.data); });

    mIpc->onEvent("openProject", [this](const webviewIpc::IPCEvent& event) { return handleOpenProject(event.data); });

    mIpc->onEvent("openFileInExplorer",
                    [this](const webviewIpc::IPCEvent& event) { return handleOpenFileInExplorer(event.data); });

    mIpc->onRequest("removeFromRecent", [this](const webviewIpc::IPCRequest& request) { return handleRemoveFromRecent(request.params); });
}

void RecentHomepageView::cleanupIPC() {}
void RecentHomepageView::showRecentFiles(int images){
    boost::property_tree::wptree data;
    wxGetApp().mainframe->get_recent_projects(data, images);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& [key, value] : data) {
        nlohmann::json fileJson;
        fileJson["path"]         = wxString(value.get<std::wstring>(L"path", L"")).ToUTF8();
        fileJson["project_name"] = wxString(value.get<std::wstring>(L"project_name", L"")).ToUTF8();
        fileJson["time"]         = wxString(value.get<std::wstring>(L"time", L"")).ToUTF8();
        fileJson["image"]        = wxString(value.get<std::wstring>(L"image", L"")).ToUTF8();
        result.push_back(fileJson);
    }
    mIpc->sendEvent("recentFilesUpdated", result);
}
webviewIpc::IPCResult RecentHomepageView::handleGetRecentFiles(const nlohmann::json&)
{
    int images= INT_MAX;
    boost::property_tree::wptree data;
    wxGetApp().mainframe->get_recent_projects(data, images);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& [key, value] : data) {
        nlohmann::json fileJson;
        fileJson["path"]         = wxString(value.get<std::wstring>(L"path", L"")).ToUTF8();
        fileJson["project_name"] = wxString(value.get<std::wstring>(L"project_name", L"")).ToUTF8();
        fileJson["time"]         = wxString(value.get<std::wstring>(L"time", L"")).ToUTF8();
        fileJson["image"]        = wxString(value.get<std::wstring>(L"image", L"")).ToUTF8();
        result.push_back(fileJson);
    }
    return webviewIpc::IPCResult::success(result);
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
        wxString wxFilePathStr = wxString::FromUTF8(filePath);
        // 打开文件夹
        wxFileName fileName(wxFilePathStr);
        wxString dirPath = fileName.GetPath();
        wxLaunchDefaultBrowser("file://" + dirPath);
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

void RecentHomepageView::OnNavigationRequest(wxWebViewEvent& evt){
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << ": " << evt.GetTarget().ToUTF8().data();
    const wxString &url = evt.GetURL();
    if (url.StartsWith("File://") || url.StartsWith("file://")) {
        if (!url.Contains("resources/web/")) {
            auto file = wxURL::Unescape(wxURL(url).GetPath());
#ifdef _WIN32
            if (file.StartsWith('/'))
                file = file.Mid(1);
            else
                file = "//" + file; // When file from network location
#endif
            wxGetApp().plater()->load_files(wxArrayString{1, &file});
            evt.Veto();
            return;
        }
    }
}
void RecentHomepageView::OnNavigationComplete(wxWebViewEvent& evt){
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << ": " << evt.GetTarget().ToUTF8().data();
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
        if (userNetworkInfo.userId.empty() || userNetworkInfo.token.empty() ||
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
    mIpc->onRequest("reload", [this](const webviewIpc::IPCRequest& request) {
        std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHostType::htElegooLink);
        if (networkHelper) {
            std::string url = networkHelper->getOnlineModelsUrl();
            mBrowser->SetUserAgent(networkHelper->getUserAgent());
            mBrowser->LoadURL(url);
        }
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

void OnlineModelsHomepageView::onWebViewError(wxWebViewEvent& evt)
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

    auto code = evt.GetInt();
    if (code == wxWEBVIEW_NAV_ERR_CONNECTION || code == wxWEBVIEW_NAV_ERR_NOT_FOUND || code == wxWEBVIEW_NAV_ERR_REQUEST) {
        loadFailedPage();
    }
}
#define FAILED_URL_SUFFIX "/web/error-page/connection-failed.html"
void OnlineModelsHomepageView::loadFailedPage()
{
    auto path = resources_dir() + FAILED_URL_SUFFIX;
    #if WIN32
        std::replace(path.begin(), path.end(), '/', '\\');
    #endif
    auto failedUrl = wxString::Format("file:///%s", from_u8(path));
    mBrowser->LoadURL(failedUrl);
}
}} // namespace Slic3r::GUI
