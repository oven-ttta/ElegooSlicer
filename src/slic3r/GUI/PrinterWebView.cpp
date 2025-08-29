#include "PrinterWebView.hpp"

#include "I18N.hpp"
#include "slic3r/GUI/PrinterWebView.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"

#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>

#include <slic3r/GUI/Widgets/WebView.hpp>
#include <wx/webview.h>
#include "slic3r/GUI/PhysicalPrinterDialog.hpp"
#include "PrinterManager.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <slic3r/Utils/WebviewIPCManager.h>

#define CONNECTIONG_URL_SUFFIX "/web/orca/connecting.html"
#define FAILED_URL_SUFFIX "/web/orca/connection-failed.html"
namespace Slic3r { namespace GUI {
PrinterWebView::PrinterWebView(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
    m_uploadInProgress = false;
    m_shouldStop       = false;

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

    // Create the webview
    m_browser = WebView::CreateWebView(this, "");
    if (m_browser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }
    this->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
    m_browser->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
    m_browser->SetOwnBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
    m_ipc = new webviewIpc::WebviewIPCManager(m_browser);
    setupIPCHandlers();
    m_browser->Bind(wxEVT_WEBVIEW_ERROR, &PrinterWebView::OnError, this);
    m_browser->Bind(wxEVT_WEBVIEW_LOADED, &PrinterWebView::OnLoaded, this);
    m_browser->Bind(wxEVT_WEBVIEW_NAVIGATING, &PrinterWebView::OnNavgating, this);
    m_browser->Bind(wxEVT_WEBVIEW_NAVIGATED, &PrinterWebView::OnNavgated, this);
    SetSizer(topsizer);
    // m_browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterWebView::OnScriptMessage, this);

    auto _dir = resources_dir();
    // replace all "\\" with "/"
    std::replace(_dir.begin(), _dir.end(), '\\', '/');

    m_connectiongUrl = wxString::Format("file:///%s" + wxString(CONNECTIONG_URL_SUFFIX), from_u8(_dir));
    m_failedUrl      = wxString::Format("file:///%s" + wxString(FAILED_URL_SUFFIX), from_u8(_dir));

    auto injectJsPath = boost::format("%1%/web/orca/inject.js") % _dir;
    // read file
    std::string   injectJs;
    std::ifstream injectJsFile(injectJsPath.str());
    if (injectJsFile.is_open()) {
        std::string line;
        while (std::getline(injectJsFile, line)) {
            injectJs += line;
        }
        injectJsFile.close();
    } else {
        wxLogError("Could not open inject.js");
    }
    //
    // #ifdef WIN32
    //    injectJs = "let isWin=true;\n" + injectJs;
    // #endif // WIN32

    // Add inject.js to the webview
    bool ret = m_browser->AddUserScript(injectJs);
    if (!ret) {
        wxLogError("Could not add user script");
    }

    topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));
    update_mode();

    // Log backend information
    /* m_browser->GetUserAgent() may lead crash
    if (wxGetApp().get_mode() == comDevelop) {
        wxLogMessage(wxWebView::GetBackendVersionInfo().ToString());
        wxLogMessage("Backend: %s Version: %s", m_browser->GetClassInfo()->GetClassName(),
            wxWebView::GetBackendVersionInfo().ToString());
        wxLogMessage("User Agent: %s", m_browser->GetUserAgent());
    }
    */

    // Zoom
    m_zoomFactor = 100;

    // Connect the idle events
    Bind(wxEVT_CLOSE_WINDOW, &PrinterWebView::OnClose, this);
}

PrinterWebView::~PrinterWebView()
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Start";
    SetEvtHandlerEnabled(false);

    // stop upload thread
    m_shouldStop = true;
    if (m_uploadThread.joinable()) {
        m_uploadThread.join();
    }

    if (m_ipc) {
        delete m_ipc;
        m_ipc = nullptr;
    }

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " End";
}

void PrinterWebView::load_url(wxString& url, wxString apikey)
{
    //    this->Show();
    //    this->Raise();
    if (m_browser == nullptr)
        return;
    m_apikey      = apikey;
    m_apikey_sent = false;
    m_url         = url;
    loadConnectingPage();
    // m_browser->SetFocus();
    UpdateState();
}
void PrinterWebView::OnNavgating(wxWebViewEvent& event)
{
    auto url = event.GetURL();
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": url %1%") % url;

    // Because the space in the browser url is escaped as %20, so when comparing strings,
    // the full path cannot be compared, use SUFFIX for comparison
    if ((m_loadState != PWLoadState::CONNECTING_LOADING && url.Contains(CONNECTIONG_URL_SUFFIX)) ||
        (m_loadState != PWLoadState::FAILED_LOADING && url.Contains(FAILED_URL_SUFFIX))) {
        m_loadState = PWLoadState::CONNECTING_LOADING;
        event.Veto();
        loadConnectingPage();
    }
}
void PrinterWebView::OnNavgated(const wxWebViewEvent& event) {}
void PrinterWebView::reload() { m_browser->Reload(); }

void PrinterWebView::update_mode() { m_browser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode")); }

/**
 * Method that retrieves the current state from the web control and updates the
 * GUI the reflect this current state.
 */
void PrinterWebView::UpdateState()
{
    // SetTitle(m_browser->GetCurrentTitle());
}

void PrinterWebView::OnClose(const wxCloseEvent& evt) { this->Hide(); }

void PrinterWebView::SendAPIKey()
{
    if (m_apikey_sent || m_apikey.IsEmpty())
        return;
    m_apikey_sent   = true;
    wxString script = wxString::Format(R"(
    // Check if window.fetch exists before overriding
    if (window.fetch) {
        const originalFetch = window.fetch;
        window.fetch = function(input, init = {}) {
            init.headers = init.headers || {};
            init.headers['X-API-Key'] = '%s';
            return originalFetch(input, init);
        };
    }
)",
                                       m_apikey);
    // m_browser->RemoveAllUserScripts();

    m_browser->AddUserScript(script);
    m_browser->Reload();
}

void PrinterWebView::OnError(const wxWebViewEvent& evt)
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

    if (message == "COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED") {
        return;
    }
    auto code = evt.GetInt();
    if (code == wxWEBVIEW_NAV_ERR_CONNECTION || code == wxWEBVIEW_NAV_ERR_NOT_FOUND || code == wxWEBVIEW_NAV_ERR_REQUEST) {
        loadFailedPage();
    }
}

void PrinterWebView::OnLoaded(const wxWebViewEvent& evt)
{
    if (m_loadState == PWLoadState::CONNECTING_LOADING) {
        m_loadState = PWLoadState::CONNECTING_LOADED;
        loadInputUrl();
    } else if (m_loadState == PWLoadState::URL_LOADING) {
        m_loadState = PWLoadState::URL_LOADED;
        if (evt.GetURL().IsEmpty())
            return;
        SendAPIKey();
    } else if (m_loadState == PWLoadState::FAILED_LOADING) {
        m_loadState = PWLoadState::FAILED_LOADED;
    }
}
void PrinterWebView::OnScriptMessage(const wxWebViewEvent& event)
{
    // #if defined(__APPLE__) || defined(__MACH__)
    wxString message = event.GetString();
    wxLogMessage("Received message: %s", message);
    // #endif
}

void PrinterWebView::loadConnectingPage()
{
    m_loadState = PWLoadState::CONNECTING_LOADING;
    loadUrl(m_connectiongUrl);
}
void PrinterWebView::loadFailedPage()
{
    if (m_loadState == PWLoadState::URL_LOADED || m_loadState == PWLoadState::URL_LOADING) {
        m_loadState = PWLoadState::FAILED_LOADING;
        // if (!m_isRetry) {
        loadUrl(m_failedUrl + "?url=" + m_url);
        //} else {
        //    m_browser->RunScript(R"(
        //                            function cancelLoading() {
        //                                console.log('cancelLoading');
        //                                window.is_loading_printer = false;
        //                                try{
        //                                    document.getElementById('reconnectButton').disabled = false;
        //                                    document.getElementById('reconnectButton').classList.remove('is-loading');
        //                                }catch(e){
        //                                }
        //                            }
        //                            cancelLoading();
        //                            )");
        //}
    }
}
void PrinterWebView::loadInputUrl()
{
    if (m_loadState == PWLoadState::CONNECTING_LOADED) {
        m_loadState = PWLoadState::URL_LOADING;
        loadUrl(m_url);
        return;
    }
}
void PrinterWebView::loadUrl(const wxString& url) { m_browser->LoadURL(url); }

void PrinterWebView::runScript(const wxString& javascript)
{
    if (!m_browser)
        return;
    WebView::RunScript(m_browser, javascript);
}

void PrinterWebView::setupIPCHandlers()
{
    if (!m_ipc)
        return;

    // handle open url request
    m_ipc->onRequest("open", [this](const webviewIpc::IPCRequest& request) {
        auto        params       = request.params;
        std::string url          = params.value("url", "");
        bool        needDownload = params.value("needDownload", false);
        wxLogMessage("Open URL: %s", url);
        if (needDownload) {
            GUI::wxGetApp().download(url);
        } else {
            wxLaunchDefaultBrowser(url);
        }
        return webviewIpc::IPCResult::success();
    });

    // handle reload request
    m_ipc->onRequest("reload", [this](const webviewIpc::IPCRequest& request) {
        auto params = request.params;
        if (!m_url.IsEmpty()) {
            m_loadState = PWLoadState::URL_LOADING;
            loadUrl(m_url);
        }
        return webviewIpc::IPCResult::success();
    });

    // handle file dialog request
    m_ipc->onRequest("open_file_dialog", [this](const webviewIpc::IPCRequest& request) {
        auto params = request.params;
        try {
            auto         filter = params.value("filter", "All files (*.*)|*.*");
            wxFileDialog openFileDialog(this, _L("Open File"), "", "", filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (openFileDialog.ShowModal() != wxID_CANCEL) {
                nlohmann::json data;
                data["files"] = {openFileDialog.GetPath().ToUTF8().data()};
                return webviewIpc::IPCResult::success(data);
            } else {
                nlohmann::json data;
                data["files"] = nlohmann::json::array();
                return webviewIpc::IPCResult::success(data);
            }
        } catch (const std::exception& e) {
            return webviewIpc::IPCResult::error("No file selected");
        }
    });

    // handle file upload request (using asynchronous event handler)
    m_ipc->onRequestAsyncWithEvents("upload_file", [this](const webviewIpc::IPCRequest&                     request,
                                                          std::function<void(const webviewIpc::IPCResult&)> sendResponse,
                                                          std::function<void(const std::string&, const nlohmann::json&)> sendEvent) mutable {
        auto params = request.params;

        // check if there is an upload in progress
        if (m_uploadInProgress) {
            sendResponse(webviewIpc::IPCResult::error("Upload already in progress"));
            return;
        }

        try {
            PrinterNetworkParams networkParams;
            networkParams.fileName  = params.value("fileName", "");
            networkParams.filePath  = params.value("filePath", "");
            networkParams.printerId = params.value("printerId", "");

            std::string requestId          = request.id;
            networkParams.uploadProgressFn = [this, requestId, sendEvent](const uint64_t uploadedBytes, const uint64_t totalBytes,
                                                                          bool& cancel) {
                // check if stop is needed
                if (m_shouldStop) {
                    cancel = true;
                    return;
                }

                nlohmann::json data;
                data["uploadedBytes"] = uploadedBytes;
                data["totalBytes"]    = totalBytes;

                // send progress event
                sendEvent("upload_progress", data);
            };

            // set upload status
            m_uploadInProgress = true;

            // if the previous thread is still running, wait for it to end
            if (m_uploadThread.joinable()) {
                m_uploadThread.join();
            }

            // start new upload thread
            m_uploadThread = std::thread([this, networkParams, requestId, sendResponse]() mutable {
                bool ret = false;
                try {
                    auto networkResult = PrinterManager::getInstance()->upload(networkParams);
                    ret                = networkResult.isSuccess();
                } catch (...) {
                    ret = false;
                }

                // reset upload status
                m_uploadInProgress = false;

                // send response in main thread
                auto response = ret ? webviewIpc::IPCResult::success() : webviewIpc::IPCResult::error("Upload failed");
                sendResponse(response);
            });
        } catch (...) {
            m_uploadInProgress = false;
            sendResponse(webviewIpc::IPCResult::error("Upload initialization failed"));
        }
    });
}

}} // namespace Slic3r::GUI
