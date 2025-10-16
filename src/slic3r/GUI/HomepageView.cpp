#include "HomepageView.hpp"
#include <wx/webview.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <slic3r/Utils/WebviewIPCManager.h>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/UserLoginView.hpp>
#include <slic3r/GUI/MainFrame.hpp>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <slic3r/Utils/PrinterManager.hpp>

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
    mIpc = new webviewIpc::WebviewIPCManager(mBrowser);
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

void RecentHomepageView::cleanupIPC()
{
    if (mIpc) {
        delete mIpc;
        mIpc = nullptr;
    }
}

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
    mIpc = new webviewIpc::WebviewIPCManager(mBrowser);
    setupIPCHandlers();

    // Load online models page from remote URL
    wxString onlineUrl = "https://np-sit.elegoo.com.cn/elegooSlicer";
    mBrowser->LoadURL(onlineUrl);
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
}

void OnlineModelsHomepageView::initialize()
{
    // Additional initialization if needed
}

void OnlineModelsHomepageView::updateMode()
{
    // Update mode if needed
}

void OnlineModelsHomepageView::setupIPCHandlers()
{
    if (!mIpc)
        return;

    mIpc->onRequest("report.getClientUserInfo", [this](const webviewIpc::IPCRequest& request) {
        UserNetworkInfo userNetworkInfo = PrinterManager::getInstance()->getIotUserInfo();
        nlohmann::json  data;
        data["userId"]       = userNetworkInfo.userId;
        data["accessToken"]  = userNetworkInfo.token;
        data["refreshToken"] = userNetworkInfo.refreshToken;
        data["expiresTime"]  = userNetworkInfo.accessTokenExpireTime;
        if (userNetworkInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            return webviewIpc::IPCResult::error(data);
        }
        return webviewIpc::IPCResult::success(data);
    });

    mIpc->onRequest("report.notLogged", [this](const webviewIpc::IPCRequest& request) { return webviewIpc::IPCResult::success(); });

    mIpc->onRequest("report.ready", [this](const webviewIpc::IPCRequest& request) {
        return handleReady();
    });

    mIpc->onRequest("report.slicerOpen", [this](const webviewIpc::IPCRequest& request) {
        auto params = request.params;
        std::string url = params.value("url", "");
        
        GUI::wxGetApp().download(url);
        
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

    nlohmann::json data;
    data["userId"]      = userNetworkInfo.userId;
    data["accessToken"] = userNetworkInfo.token;
    data["avatar"]      = userNetworkInfo.avatar;
    data["email"]       = userNetworkInfo.email;
    data["nickname"]    = userNetworkInfo.nickname;
    mIpc->sendEvent("client.onUserInfoUpdated", data, mIpc->generateRequestId());
    wxLogMessage("OnlineModelsHomepageView: Sent user info to WebView");
}
void OnlineModelsHomepageView::cleanupIPC()
{
    if (mIpc) {
        delete mIpc;
        mIpc = nullptr;
    }
}

webviewIpc::IPCResult OnlineModelsHomepageView::handleReady()
{
    lock_guard<mutex> lock(mUserInfoMutex);
    mIsReady = true;
    if (mIpc && !mRefreshUserInfo.userId.empty()) {
        nlohmann::json data;
        data["userId"]      = mRefreshUserInfo.userId;
        data["accessToken"] = mRefreshUserInfo.token;
        data["avatar"]      = mRefreshUserInfo.avatar;
        data["email"]       = mRefreshUserInfo.email;
        data["nickname"]    = mRefreshUserInfo.nickname;
        mIpc->sendEvent("client.onUserInfoUpdated", data, mIpc->generateRequestId());
        wxLogMessage("OnlineModelsHomepageView: Sent user info to WebView");
        mRefreshUserInfo = UserNetworkInfo();
    }
    return webviewIpc::IPCResult::success();
}
void OnlineModelsHomepageView::onWebViewLoaded(wxWebViewEvent& event)
{

}

void OnlineModelsHomepageView::onWebViewError(wxWebViewEvent& event)
{
    wxString error = event.GetString();
    wxMessageBox("WebView Error: " + error, "Error", wxOK | wxICON_ERROR);
}

}} // namespace Slic3r::GUI
