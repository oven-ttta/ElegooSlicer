#include "PrinterManager.hpp"
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
#include <boost/beast/core/detail/base64.hpp>
#include "slic3r/Utils/PrinterManager.hpp"
#include "slic3r/Utils/PrintHost.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include <map>
#include <algorithm>
#include <boost/filesystem.hpp>

#define FIRST_TAB_NAME _L("Connected Printer")

namespace Slic3r {
namespace GUI {

std::string imageFileToBase64DataURI(const std::string& image_path) {
    std::ifstream ifs(image_path, std::ios::binary);
    if (!ifs) return "";
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string img_data = oss.str();
    if (img_data.empty()) return "";
    std::string encoded;
    encoded.resize(boost::beast::detail::base64::encoded_size(img_data.size()));
    encoded.resize(boost::beast::detail::base64::encode(
        &encoded[0], img_data.data(), img_data.size()));
    std::string ext = "png";
    size_t dot = image_path.find_last_of('.');
    if (dot != std::string::npos) {
        ext = image_path.substr(dot + 1);
        boost::algorithm::to_lower(ext);
    }
    return "data:image/" + ext + ";base64," + encoded;
}

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
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
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
    mTabBar->AddPage(mBrowser, FIRST_TAB_NAME);

    auto _dir = resources_dir();
    std::replace(_dir.begin(), _dir.end(), '\\', '/');
    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/printer_manager/printer.html").make_preferred().string());
    TargetUrl = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);
    mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterManagerView::onScriptMessage, this);
    mTabBar->SetSelection(0);
    Bind(wxEVT_CLOSE_WINDOW, &PrinterManagerView::onClose, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PrinterManagerView::onClosePrinterTab, this);
}

PrinterManagerView::~PrinterManagerView() {

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
        url = url + wxString("?id=") + from_u8(printerInfo.printerId) + "&ip=" + printerInfo.host +"&sn=" + from_u8(printerInfo.serialNumber);
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
void PrinterManagerView::onScriptMessage(wxWebViewEvent& event)
{
    wxString message = event.GetString();
    try {
        nlohmann::json root = nlohmann::json::parse(message.ToUTF8().data());
        std::string cmd = root["command"];  
        handleCommand(cmd, root);
        
    } catch (std::exception& e) {
        wxLogMessage("Error: %s", e.what());
    }
}

void PrinterManagerView::handleCommand(const std::string& cmd, const nlohmann::json& root)
{
    if (cmd == "request_printer_list") {
        sendResponse("response_printer_list", "10001", getPrinterList());
    } else if (cmd == "request_printer_model_list") {
        sendResponse("response_printer_model_list", "10002", getPrinterModelList());
    } else if (cmd == "request_printer_detail") {
        openPrinterTab(root["id"]);
    } else if (cmd == "request_discover_printers") {
        sendResponse("response_discover_printers", "10003", discoverPrinter());
    } else if (cmd == "request_add_printer") {
        nlohmann::json printer = root["printer"];
        sendResponse("response_add_printer", "10004", addPrinter(printer));
    } else if (cmd == "request_add_physical_printer") {
        sendResponse("response_add_physical_printer", "10005", addPhysicalPrinter(root["printer"]));
    } else if (cmd == "request_update_printer_name") {
        sendResponse("response_update_printer_name", "10005", 
                    updatePrinterName(root["printerId"], root["printerName"]));
    } else if (cmd == "request_update_printer_host") {
        sendResponse("response_update_printer_host", "10007", 
                    updatePrinterHost(root["printerId"], root["host"]));
    } else if (cmd == "request_delete_printer") {
        sendResponse("response_delete_printer", "10006", deletePrinter(root["printerId"]));
    } else if (cmd == "request_browse_ca_file") {
        sendResponse("response_browse_ca_file", "10008", browseCAFile());
    } else if (cmd == "request_refresh_printers") {
       
    } else if (cmd == "request_logout_print_host") {
     
    } else if (cmd == "request_connect_physical_printer") {
     
    } else {
        wxLogWarning("Unknown command: %s", cmd);
    }
}

void PrinterManagerView::sendResponse(const std::string& command, const std::string& sequenceId, const nlohmann::json& response)
{
    nlohmann::json jsonResponse = {
        {"command", command},
        {"sequence_id", sequenceId},
        {"response", response}
    };
    
    wxString strJS = wxString::Format("HandleStudio(%s)", jsonResponse.dump(-1, ' ', true));
    wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
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
    PrinterNetworkInfo printerInfo = PrinterManager::convertJsonToPrinterNetworkInfo(printer);
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
        printer_obj = PrinterManager::convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printer_obj["printerImg"] = imageFileToBase64DataURI(img_path);
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
        printer_obj = PrinterManager::convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printer_obj["printerImg"] = imageFileToBase64DataURI(img_path);
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


