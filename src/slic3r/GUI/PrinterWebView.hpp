#ifndef slic3r_PrinterWebView_hpp_
#define slic3r_PrinterWebView_hpp_


#include "wx/artprov.h"
#include "wx/cmdline.h"
#include "wx/notifmsg.h"
#include "wx/settings.h"
#include <wx/webview.h>
#include <wx/string.h>
#include <thread>
#include <mutex>
#include <atomic>

#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif

#include "wx/webviewarchivehandler.h"
#include "wx/webviewfshandler.h"
#include "wx/numdlg.h"
#include "wx/infobar.h"
#include "wx/filesys.h"
#include "wx/fs_arc.h"
#include "wx/fs_mem.h"
#include "wx/stdpaths.h"
#include <wx/panel.h>
#include <wx/tbarbase.h>
#include "wx/textctrl.h"
#include <wx/timer.h>


namespace Slic3r {
namespace GUI {


class PrinterWebView : public wxPanel {
public:
    PrinterWebView(wxWindow *parent);
    virtual ~PrinterWebView();

    void load_url(wxString& url, wxString apikey = "");
    void UpdateState();
    void OnClose(wxCloseEvent& evt);
    void OnError(wxWebViewEvent& evt);
    void OnLoaded(wxWebViewEvent& evt);
    void reload();
    void update_mode();
private:
    void SendAPIKey();
    void OnScriptMessage(wxWebViewEvent& event);
    void       OnNavgating(wxWebViewEvent& event);
    void       OnNavgated(wxWebViewEvent& event);
    void       loadConnectingPage();
    void       loadFailedPage();
    void       loadInputUrl();
    void       loadUrl(const wxString& url);
    void runScript(const wxString &javascript);

    wxWebView* m_browser;
    long m_zoomFactor;
    wxString m_apikey;
    bool m_apikey_sent;
    // DECLARE_EVENT_TABLE()

    wxString m_url;
    wxString m_connectiongUrl;
    wxString m_failedUrl;
    enum PWLoadState { 
        CONNECTING_LOADING = 0, 
        CONNECTING_LOADED, 
        URL_LOADING, 
        URL_LOADED, 
        FAILED_LOADING, 
        FAILED_LOADED 
    };
    // 0 is load connecting page
    // 1 is load url
    // 2 is load failed page
    PWLoadState m_loadState = PWLoadState::CONNECTING_LOADING;
    
    // Upload thread management
    std::atomic<bool> m_uploadInProgress;
    std::atomic<bool> m_shouldStop;
    std::thread m_uploadThread;
    std::mutex m_uploadMutex;
};

} // GUI
} // Slic3r

#endif /* slic3r_Tab_hpp_ */
