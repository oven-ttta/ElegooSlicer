#ifndef HOMEVIEW_HPP
#define HOMEVIEW_HPP

#include <wx/wx.h>
#include <wx/webview.h>
#include <memory>
#include <map>
#include "slic3r/Utils/WebviewIPCManager.h"

namespace Slic3r { namespace GUI {

// Forward declarations
class HomepageView;

class HomeView : public wxPanel
{
public:
    HomeView(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = "Home");
    ~HomeView();

    void sendRecentList(int images);
    void updateMode();
    void switchToPage(const wxString& pageName);
    void refreshUserInfo();
    
private:
    void initUI();
    void setupIPCHandlers();
    void cleanupIPC();
    void createHomepageViews();
    void showPage(const wxString& pageName);
    
    // IPC handlers
    webviewIpc::IPCResult handleGetUserInfo();
    webviewIpc::IPCResult handleLogout();
    webviewIpc::IPCResult handleNavigateToPage(const nlohmann::json& data);
    webviewIpc::IPCResult handleShowLoginDialog();
    
    // Event handlers
    void onWebViewLoaded(wxWebViewEvent& event);
    void onWebViewError(wxWebViewEvent& event);
    
private:
    // UI Components
    wxBoxSizer* mMainSizer;
    wxWebView* mNavigationBrowser;  // Left navigation webview
    wxStaticLine* mDividerLine;     // Vertical divider line
    wxPanel* mContentPanel;         // Right content panel
    wxBoxSizer* mContentSizer;     // Sizer for content panel
    
    // IPC
    webviewIpc::WebviewIPCManager* mIpc;
    
    // Homepage views
    std::map<wxString, HomepageView*> mHomepageViews;
    HomepageView* mCurrentView;
    
    DECLARE_EVENT_TABLE()
};

}} // namespace Slic3r::GUI

#endif // HOMEVIEW_HPP
