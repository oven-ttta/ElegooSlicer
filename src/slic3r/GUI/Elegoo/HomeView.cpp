#include "HomeView.hpp"
#include "HomepageView.hpp"
#include <wx/webview.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <slic3r/Utils/WebviewIPCManager.h>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/Elegoo/UserLoginView.hpp>
#include <slic3r/GUI/MainFrame.hpp>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <slic3r/Utils/Elegoo/UserNetworkManager.hpp>
#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI {

wxBEGIN_EVENT_TABLE(HomeView, wxPanel) EVT_WEBVIEW_LOADED(wxID_ANY, HomeView::onWebViewLoaded)
    EVT_WEBVIEW_ERROR(wxID_ANY, HomeView::onWebViewError) wxEND_EVENT_TABLE()

        HomeView::HomeView(wxWindow* parent, wxWindowID id, const wxString& title)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize)
    , mMainSizer(nullptr)
    , mNavigationBrowser(nullptr)
    , mDividerLine(nullptr)
    , mContentPanel(nullptr)
    , mContentSizer(nullptr)
    , mIpc(nullptr)
    , mCurrentView(nullptr)
{
    initUI();
}

HomeView::~HomeView()
{
    cleanupIPC();

    // Clean up homepage views
    for (auto& pair : mHomepageViews) {
        delete pair.second;
    }
    mHomepageViews.clear();
}

void HomeView::initUI()
{
    // Create main horizontal sizer
    mMainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create navigation webview (left side) with fixed width
    mNavigationBrowser = WebView::CreateWebView(this, "");

    if (!mNavigationBrowser) {
        wxMessageBox("Failed to create navigation webview", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Remove WebView border completely
    mNavigationBrowser->SetWindowStyleFlag(wxNO_BORDER);

    // Create vertical divider line
    mDividerLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

    // Set lighter color for the divider line
    mDividerLine->SetForegroundColour(wxColour(230, 230, 230)); // Light gray color

    // Create content panel (right side) - this will contain different HomepageViews
    mContentPanel = new wxPanel(this, wxID_ANY);
    mContentSizer = new wxBoxSizer(wxVERTICAL);
    mContentPanel->SetSizer(mContentSizer);

    // Add to main sizer with divider line
    mMainSizer->Add(mNavigationBrowser, 0, wxEXPAND | wxALL, 0);
    mMainSizer->Add(mDividerLine, 0, wxEXPAND | wxTOP | wxBOTTOM, 0);
    mMainSizer->Add(mContentPanel, 1, wxEXPAND, 0);

    // Set minimum size for navigation browser
    mNavigationBrowser->SetMinSize(wxSize(240, -1));
    SetSizer(mMainSizer);

    // Initialize IPC for navigation
    mIpc = std::make_unique<webviewIpc::WebviewIPCManager>(mNavigationBrowser);
    setupIPCHandlers();

    // Create homepage views
    createHomepageViews();

    // Load navigation page (left side) - only navigation bar
    wxString navUrl = wxString::Format("file://%s/web/homepage4/navigation.html", from_u8(resources_dir()));
    mNavigationBrowser->LoadURL(navUrl);

    mNavigationBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));

    // Show default page
    showPage("recent");
}

void HomeView::createHomepageViews()
{
    // Create Recent homepage view
    mHomepageViews["recent"] = new RecentHomepageView(mContentPanel);
    mContentSizer->Add(mHomepageViews["recent"], 1, wxEXPAND | wxALL, 0);
    mHomepageViews["recent"]->Show(false); // Initially hidden

    // Create Online Models homepage view
    mHomepageViews["online-models"] = new OnlineModelsHomepageView(mContentPanel);
    mContentSizer->Add(mHomepageViews["online-models"], 1, wxEXPAND | wxALL, 0);
    mHomepageViews["online-models"]->Show(false); // Initially hidden

    // Initialize all views
    for (auto& pair : mHomepageViews) {
        pair.second->initialize();
    }
}

void HomeView::showPage(const wxString& pageName)
{
    wxLogMessage("showPage called with: %s", pageName);

    // Hide all views first
    for (auto& pair : mHomepageViews) {
        pair.second->Show(false);
        wxLogMessage("Hiding view: %s", pair.first);
    }

    // Show the selected view
    auto it = mHomepageViews.find(pageName);
    if (it != mHomepageViews.end()) {
        mCurrentView = it->second;
        mCurrentView->Show(true);
        mContentSizer->Layout();
        wxLogMessage("Showing view: %s", pageName);
    } else {
        wxLogMessage("View not found: %s", pageName);
    }
}

void HomeView::switchToPage(const wxString& pageName) { showPage(pageName); }

void HomeView::setupIPCHandlers()
{
    if (!mIpc)
        return;

    // Navigation handler - this is the key handler for switching pages
    mIpc->onRequest("navigateToPage", [this](const webviewIpc::IPCRequest& request) { return handleNavigateToPage(request.params); });
    mIpc->onRequest("getUserInfo", [this](const webviewIpc::IPCRequest& request) { return handleGetUserInfo(); });
    mIpc->onRequest("logout", [this](const webviewIpc::IPCRequest& request) { return handleLogout(); });
    mIpc->onRequest("showLoginDialog", [this](const webviewIpc::IPCRequest& request) { return handleShowLoginDialog(); });
    mIpc->onRequest("ready", [this](const webviewIpc::IPCRequest& request) { return handleReady(); });
}

void HomeView::cleanupIPC()
{
}

webviewIpc::IPCResult HomeView::handleNavigateToPage(const nlohmann::json& data)
{
    std::string pageName   = data.value("page", "recent");
    wxString    wxPageName = wxString::FromUTF8(pageName);

    // Debug: Print the page name
    wxLogMessage("Navigating to page: %s", wxPageName);

    // Check if the page exists
    auto it = mHomepageViews.find(wxPageName);
    if (it == mHomepageViews.end()) {
        wxLogMessage("Page not found: %s", wxPageName);
        return webviewIpc::IPCResult::success();
    }

    switchToPage(wxPageName);
    return webviewIpc::IPCResult::success();
}

webviewIpc::IPCResult HomeView::handleGetUserInfo()
{
    // Get user network info
    UserNetworkInfo userNetworkInfo = UserNetworkManager::getInstance()->getUserInfo();   
    nlohmann::json data; 
    data = convertUserNetworkInfoToJson(userNetworkInfo);
    return webviewIpc::IPCResult::success(data);
}
webviewIpc::IPCResult HomeView::handleLogout() { 
    UserNetworkManager::getInstance()->setUserInfo(UserNetworkInfo());
    return webviewIpc::IPCResult::success(); 
}

webviewIpc::IPCResult HomeView::handleReady()
{
    lock_guard<mutex> lock(mUserInfoMutex);
    mIsReady = true;
    // Send any pending user info with delay to ensure frontend is ready
    if(!mRefreshUserInfo.userId.empty() && mIpc) {
        nlohmann::json data = convertUserNetworkInfoToJson(mRefreshUserInfo);
        if(UserNetworkManager::getInstance()->isOnline(mRefreshUserInfo)) {
            data["online"] = true;
        } else {
            data["online"] = false;
        }
        mIpc->sendEvent("onUserInfoUpdated", data, mIpc->generateRequestId());
        mRefreshUserInfo = UserNetworkInfo();
        wxLogMessage("HomeView: Sent pending user info to WebView");
    }
    wxLogMessage("HomeView: Ready");  
    return webviewIpc::IPCResult::success();
}

void HomeView::onWebViewLoaded(wxWebViewEvent& event)
{

}

void HomeView::onWebViewError(wxWebViewEvent& event)
{
    wxString error = event.GetString();
    wxMessageBox("WebView Error: " + error, "Error", wxOK | wxICON_ERROR);
}

void HomeView::sendRecentList(int images)
{
    //     boost::property_tree::wptree req;
    //     boost::property_tree::wptree data;
    //     wxGetApp().mainframe->get_recent_projects(data, images);
    //     req.put(L"sequence_id", "");
    //     req.put(L"command", L"get_recent_projects");
    //     req.put_child(L"response", data);
    //     std::wostringstream oss;
    //     boost::property_tree::write_json(oss, req, false);
    //     RunScript(wxString::Format("window.postMessage(%s)", oss.str()));
    // }
}

webviewIpc::IPCResult HomeView::handleShowLoginDialog()
{
    // Send event to MainFrame to show login dialog
    auto evt = new wxCommandEvent(EVT_USER_LOGIN);
    wxQueueEvent(wxGetApp().mainframe, evt);
    
    return webviewIpc::IPCResult::success();
}


void HomeView::updateMode()
{
    // Update mode if needed
}

// void HomeView::RunScript(const wxString& javascript)
// {
//     if (m_browser) {
//         m_browser->RunScript(javascript);
//     }
// }

void HomeView::refreshUserInfo()
{
    lock_guard<mutex> lock(mUserInfoMutex);
    UserNetworkInfo userNetworkInfo = UserNetworkManager::getInstance()->getUserInfo();
    if (mIpc && mIsReady) {
    // Send refresh signal to navigation webview via IPC
        nlohmann::json data = convertUserNetworkInfoToJson(userNetworkInfo);
        if(UserNetworkManager::getInstance()->isOnline(userNetworkInfo)) {
            data["online"] = true;
        } else {
            data["online"] = false;
        }
        mIpc->sendEvent("onUserInfoUpdated", data, mIpc->generateRequestId());
    } else {
        mRefreshUserInfo = userNetworkInfo;
    }
    //send event to homepage views
    for (auto& pair : mHomepageViews) {
        pair.second->onUserInfoUpdated(userNetworkInfo);
    }
}

}} // namespace Slic3r::GUI
