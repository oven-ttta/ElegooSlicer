#include "PrinterManagerView.hpp"
#include "I18N.hpp"
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
#include "slic3r/Utils/PrinterManager.hpp"
#include "slic3r/Utils/PrintHost.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include <map>
#include <algorithm>
#include <thread>
#include <boost/filesystem.hpp>
#include "slic3r/Utils/WebviewIPCManager.h"

#define FIRST_TAB_NAME _L("Connected Printer")

namespace Slic3r {
namespace GUI {

class TabArt : public wxAuiDefaultTabArt
{
public:
    wxAuiTabArt* Clone() override { return new TabArt(*this); }
    void         DrawTab(wxDC&                    dc,
                         wxWindow*                wnd,
                         const wxAuiNotebookPage& page,
                         const wxRect&            in_rect,
                         int                      close_button_state,
                         wxRect*                  out_tab_rect,
                         wxRect*                  out_button_rect,
                         int*                     x_extent) override
    {
        bool isFirstTab = (page.caption == FIRST_TAB_NAME);
        
        if (isFirstTab) {
            
            wxAuiDefaultTabArt::DrawTab(dc, wnd, page, in_rect,
                wxAUI_BUTTON_STATE_HIDDEN, out_tab_rect,
                out_button_rect, x_extent);
        } else {
            wxAuiDefaultTabArt::DrawTab(dc, wnd, page, in_rect,
                close_button_state, out_tab_rect,
                out_button_rect, x_extent);
        }
    }
};


PrinterManagerView::PrinterManagerView(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), 
      m_isDestroying(false),
      m_lifeTracker(std::make_shared<bool>(true))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    mTabBar = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_CLOSE_ON_ALL_TABS);
    mTabBar->SetArtProvider(new TabArt());
    mTabBar->SetBackgroundColour(wxColour("#FEFFFF"));
    mainSizer->Add(mTabBar, 1, wxEXPAND);
    SetSizer(mainSizer);

    mBrowser = WebView::CreateWebView(mTabBar, "");
    if (mBrowser == nullptr) {
        wxLogError("Could not init mBrowser");
        return;
    }
    
    m_ipc = new webviewIpc::WebviewIPCManager(mBrowser);
    setupIPCHandlers();
    
    mTabBar->AddPage(mBrowser, FIRST_TAB_NAME);
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
    auto _dir = resources_dir();
    std::replace(_dir.begin(), _dir.end(), '\\', '/');
    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/printer_manager/printer.html").make_preferred().string());
    TargetUrl = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);
    mTabBar->SetSelection(0);
    Bind(wxEVT_CLOSE_WINDOW, &PrinterManagerView::onClose, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PrinterManagerView::onClosePrinterTab, this);
}

PrinterManagerView::~PrinterManagerView() {
    m_isDestroying = true;
    
    // Reset the life tracker to signal all async operations that this object is being destroyed
    m_lifeTracker.reset();
    
    if (m_ipc) {
        delete m_ipc;
        m_ipc = nullptr;
    }
}

void PrinterManagerView::openPrinterTab(const std::string& printerId)
{
    auto it = mPrinterViews.find(printerId);
    if (it != mPrinterViews.end()) {
        int idx = mTabBar->GetPageIndex(it->second);
        if (idx != wxNOT_FOUND) {
            mTabBar->SetSelection(idx);
            Layout();
            return;
        }
    }
    std::vector<PrinterNetworkInfo> printerList = PrinterManager::getInstance()->getPrinterList();
    PrinterNetworkInfo printerInfo;
    for (auto& printer : printerList) {
        if (printer.printerId == printerId) {
            printerInfo = printer;
            break;  
        }
    }
    if (printerInfo.printerId.empty()) {
        wxLogError("Printer %s not found", printerId.c_str());
        return;
    }
    PrinterWebView* view = new PrinterWebView(mTabBar);
    wxString url = printerInfo.webUrl;
    
    if(printerInfo.deviceType == 2) 
    {
        std::string accessToken="123456";
        if (printerInfo.authMode == "basic") {
    
        } else if (printerInfo.authMode == "token") {
            try {
                nlohmann::json extraInfo = nlohmann::json::parse(printerInfo.extraInfo);
                if (extraInfo.contains(PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN)) {
                accessToken = extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN].get<std::string>();
                } else {
                    wxLogError("Error connecting to device: %s", "Token is not set");
                }
            }catch (nlohmann::json::parse_error& e) {
                wxLogError("Error parsing extraInfo: %s", e.what());
            }
        }
        url = url + wxString("?id=") + from_u8(printerInfo.printerId) + "&ip=" + printerInfo.host +"&sn=" + from_u8(printerInfo.serialNumber) + "&access_code=" + accessToken;
    }

    view->load_url(url);
    mTabBar->AddPage(view, from_u8(printerInfo.printerName));
    mTabBar->SetSelection(mTabBar->GetPageCount() - 1);
    mPrinterViews[printerId] = view;
    Layout();
}

void PrinterManagerView::runScript(const wxString& javascript)
{
    if (!mBrowser)
        return;
    WebView::RunScript(mBrowser, javascript);
}

void PrinterManagerView::onClose(wxCloseEvent& evt)
{
    this->Hide();
}

void PrinterManagerView::onClosePrinterTab(wxAuiNotebookEvent& event)
{
    int page = event.GetSelection();
    if (page == wxNOT_FOUND) return;

    wxWindow* win = mTabBar->GetPage(page);
    for (auto it = mPrinterViews.begin(); it != mPrinterViews.end(); ++it) {
        if (it->second == win) {
            it->second->OnClose(wxCloseEvent());
            mPrinterViews.erase(it);
            mTabBar->SetSelection(0);
            break;
        }
    }
    event.Skip();
}
void PrinterManagerView::setupIPCHandlers()
{
    if (!m_ipc) return;

    // Handle request_printer_list
    m_ipc->onRequest("request_printer_list", [this](const webviewIpc::IPCRequest& request){
        return webviewIpc::IPCResult::success(getPrinterList());
    });

    // Handle request_printer_model_list
    m_ipc->onRequest("request_printer_model_list", [this](const webviewIpc::IPCRequest& request){
        return webviewIpc::IPCResult::success(getPrinterModelList());
    });

    // Handle request_printer_list_status
    m_ipc->onRequest("request_printer_list_status", [this](const webviewIpc::IPCRequest& request){
        return webviewIpc::IPCResult::success(getPrinterListStatus());
    });

    // Handle request_printer_detail
    m_ipc->onRequest("request_printer_detail", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        if (!printerId.empty()) {
            openPrinterTab(printerId);
        }
        return webviewIpc::IPCResult::success();
    });

    // Handle request_discover_printers (async due to time-consuming operation)
    m_ipc->onRequestAsync("request_discover_printers", [this](const webviewIpc::IPCRequest& request, 
                                                           std::function<void(const webviewIpc::IPCResult&)> sendResponse) {  
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;   
        // Run the discovery operation in a separate thread to avoid blocking the UI
        std::thread([life_tracker, sendResponse,this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    // Call the static method to avoid accessing this pointer
                    auto printerList = this->discoverPrinter();
                    // Final check before sending response
                    if (life_tracker.lock()) {
                        sendResponse(webviewIpc::IPCResult::success(printerList));
                    }
                } else {
                    // Object was destroyed, don't send response
                    return;
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error(-1, std::string("Discovery failed: ") + e.what()));
                }
            }
        }).detach();
    });

    // Handle request_add_printer
    m_ipc->onRequest("request_add_printer", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        if (params.contains("printer")) {
            nlohmann::json printer = params["printer"];
            std::string result = addPrinter(printer);
            return webviewIpc::IPCResult::success(nlohmann::json(result));
        }
        return webviewIpc::IPCResult::error("Missing printer parameter");
    });

    // Handle request_add_physical_printer
    m_ipc->onRequest("request_add_physical_printer", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        if (params.contains("printer")) {
            std::string result = addPhysicalPrinter(params["printer"]);
            return webviewIpc::IPCResult::success(nlohmann::json(result));
        }
        return webviewIpc::IPCResult::error("Missing printer parameter");
    });

    // Handle request_update_printer_name
    m_ipc->onRequest("request_update_printer_name", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        std::string printerName = params.value("printerName", "");
        bool result = updatePrinterName(printerId, printerName);
        return webviewIpc::IPCResult::success(nlohmann::json(result));
    });

    // Handle request_update_printer_host
    m_ipc->onRequest("request_update_printer_host", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        std::string host = params.value("host", "");
        bool result = updatePrinterHost(printerId, host);
        return webviewIpc::IPCResult::success(nlohmann::json(result));
    });

    // Handle request_delete_printer
    m_ipc->onRequest("request_delete_printer", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        bool result = deletePrinter(printerId);
        return webviewIpc::IPCResult::success(nlohmann::json(result));
    });

    // Handle request_browse_ca_file
    m_ipc->onRequest("request_browse_ca_file", [this](const webviewIpc::IPCRequest& request){
        std::string result = browseCAFile();
        return webviewIpc::IPCResult::success(nlohmann::json(result));
    });

    // Handle request_refresh_printers
    m_ipc->onRequest("request_refresh_printers", [this](const webviewIpc::IPCRequest& request){
        // Implementation for refresh printers
        return webviewIpc::IPCResult::success();
    });

    // Handle request_logout_print_host
    m_ipc->onRequest("request_logout_print_host", [this](const webviewIpc::IPCRequest& request){
        // Implementation for logout print host
        return webviewIpc::IPCResult::success();
    });

    // Handle request_connect_physical_printer
    m_ipc->onRequest("request_connect_physical_printer", [this](const webviewIpc::IPCRequest& request){
        // Implementation for connect physical printer
        return webviewIpc::IPCResult::success();
    });
}



bool PrinterManagerView::deletePrinter(const std::string& printerId)
{
    auto it = mPrinterViews.find(printerId);
    if (it != mPrinterViews.end()) {
        int page = mTabBar->GetPageIndex(it->second);
        if (page != wxNOT_FOUND) {
            mTabBar->DeletePage(page);
        }
        it->second->OnClose(wxCloseEvent());
        mPrinterViews.erase(it);
        mTabBar->SetSelection(0);
    }
    return PrinterManager::getInstance()->deletePrinter(printerId);
}
bool PrinterManagerView::updatePrinterName(const std::string& printerId, const std::string& printerName)
{
    auto it = mPrinterViews.find(printerId);
    if (it != mPrinterViews.end()) {
        int page = mTabBar->GetPageIndex(it->second);
        if (page != wxNOT_FOUND) {
            mTabBar->SetPageText(page, from_u8(printerName));
        }
    }
    return PrinterManager::getInstance()->updatePrinterName(printerId, printerName);
}
bool PrinterManagerView::updatePrinterHost(const std::string& id, const std::string& host)
{
    return PrinterManager::getInstance()->updatePrinterHost(id, host);
}
std::string PrinterManagerView::addPrinter(const nlohmann::json& printer)
{
    PrinterNetworkInfo printerInfo = convertJsonToPrinterNetworkInfo(printer);
    printerInfo.isPhysicalPrinter = false;
    return PrinterManager::getInstance()->addPrinter(printerInfo);
}
std::string PrinterManagerView::addPhysicalPrinter(const nlohmann::json& printer)
{
    PrinterNetworkInfo printerInfo;
    try {
        printerInfo.isPhysicalPrinter = true;
        printerInfo.hostType = printer["hostType"];
        printerInfo.host = printer["host"];
        printerInfo.printerName = printer["printerName"];
        printerInfo.vendor = printer["vendor"];
        printerInfo.printerModel = printer["printerModel"];
    } catch (const std::exception& e) {
        wxLogMessage("Add physical printer error: %s", e.what());
        return "";
    }
    return PrinterManager::getInstance()->addPrinter(printerInfo);
}
nlohmann::json PrinterManagerView::discoverPrinter()
{
    auto printerList = PrinterManager::getInstance()->discoverPrinter();
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printer_obj["printerImg"] = PrinterManager::imageFileToBase64DataURI(img_path);
        response.push_back(printer_obj);
    }
    return response;
}
nlohmann::json PrinterManagerView::getPrinterList()
{  
    auto printerList = PrinterManager::getInstance()->getPrinterList();
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printer_obj["printerImg"] = PrinterManager::imageFileToBase64DataURI(img_path);
        response.push_back(printer_obj);
    }
    return response;
}
nlohmann::json PrinterManagerView::getPrinterListStatus()
{
    auto printerList = PrinterManager::getInstance()->getPrinterList();
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        response.push_back(printer_obj);
    }
    return response;
}
std::string PrinterManagerView::browseCAFile()
{
    try {
        static const auto filemasks = _L("Certificate files (*.crt, *.pem)|*.crt;*.pem|All files|*.*");
        wxFileDialog openFileDialog(this, _L("Open CA certificate file"), "", "", filemasks, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() != wxID_CANCEL) {
            return openFileDialog.GetPath().ToStdString();
        }
    } catch (const std::exception& e) {
        wxLogMessage("Browse CA file error: %s", e.what());
    }
    return "";
}
nlohmann::json PrinterManagerView::getPrinterModelList()
{
    auto vendorPrinterModelConfigMap = PrinterManager::getVendorPrinterModelConfig();   
    nlohmann::json response = nlohmann::json::array();
    for (auto& vendor : vendorPrinterModelConfigMap) {
        nlohmann::json vendorObj = nlohmann::json::object();
        vendorObj["vendor"]      = vendor.first;
        vendorObj["models"]      = nlohmann::json::array();
        for (auto& model : vendor.second) {
            nlohmann::json modelObj = nlohmann::json::object();
            modelObj["modelName"]  = model.first;
            auto config = model.second;
            const auto opt = config.option<ConfigOptionEnum<PrintHostType>>("host_type");
            const auto hostType = opt != nullptr ? opt->value : htOctoPrint;
            std::string hostTypeStr = PrintHost::get_print_host_type_str(hostType);
            if (!hostTypeStr.empty()) {
                modelObj["hostType"]   = hostTypeStr;
            }
            vendorObj["models"].push_back(modelObj);
        }
        response.push_back(vendorObj);
    }
    return response;
}
} // GUI
} // Slic3r


