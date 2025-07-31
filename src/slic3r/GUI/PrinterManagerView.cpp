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
#include "slic3r/GUI/PrintHostDialogs.hpp"
#include "slic3r/GUI/BonjourDialog.hpp"
#include "slic3r/GUI/OAuthDialog.hpp"
#include "slic3r/GUI/PrinterCloudAuthDialog.hpp"
#include "slic3r/Utils/PrintHost.hpp"
#include "slic3r/Utils/SimplyPrint.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/Utils.hpp"
#include <map>
#include <algorithm>
#include <boost/filesystem.hpp>

#define FIRST_TAB_NAME _L("Connected Printer")

namespace Slic3r {
namespace GUI {

PrinterNetworkInfo convertJsonToPrinterNetworkInfo(const nlohmann::json& json)
{
    try {
        PrinterNetworkInfo printerNetworkInfo;
        printerNetworkInfo.id                = json["id"];
        printerNetworkInfo.name              = json["name"];
        printerNetworkInfo.ip                = json["ip"];
        printerNetworkInfo.port              = json["port"].get<int>();
        printerNetworkInfo.vendor            = json["vendor"];
        printerNetworkInfo.machineName       = json["machineName"];
        printerNetworkInfo.machineModel      = json["machineModel"];
        printerNetworkInfo.protocolVersion   = json["protocolVersion"];
        printerNetworkInfo.firmwareVersion   = json["firmwareVersion"];
        printerNetworkInfo.deviceId          = json["deviceId"];
        printerNetworkInfo.deviceType        = json["deviceType"];
        printerNetworkInfo.connectionUrl     = json["connectionUrl"];
        printerNetworkInfo.serialNumber      = json["serialNumber"];
        printerNetworkInfo.webUrl            = json["webUrl"];
        printerNetworkInfo.isPhysicalPrinter = json["isPhysicalPrinter"].get<bool>();
        printerNetworkInfo.addTime           = json["addTime"].get<uint64_t>();
        printerNetworkInfo.modifyTime        = json["modifyTime"].get<uint64_t>();
        printerNetworkInfo.lastActiveTime    = json["lastActiveTime"].get<uint64_t>();
        return printerNetworkInfo;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to convert json to printer network info: " + std::string(e.what()));
    }
    return PrinterNetworkInfo();
}
nlohmann::json convertPrinterNetworkInfoToJson(const PrinterNetworkInfo& printerNetworkInfo)
{
    nlohmann::json json;
    json["id"]                = printerNetworkInfo.id;
    json["name"]              = printerNetworkInfo.name;
    json["ip"]                = printerNetworkInfo.ip;
    json["port"]              = printerNetworkInfo.port;
    json["vendor"]            = printerNetworkInfo.vendor;
    json["machineName"]       = printerNetworkInfo.machineName;
    json["machineModel"]      = printerNetworkInfo.machineModel;
    json["protocolVersion"]   = printerNetworkInfo.protocolVersion;
    json["firmwareVersion"]   = printerNetworkInfo.firmwareVersion;
    json["deviceId"]          = printerNetworkInfo.deviceId;
    json["deviceType"]        = printerNetworkInfo.deviceType;
    json["connectionUrl"]     = printerNetworkInfo.connectionUrl;
    json["serialNumber"]      = printerNetworkInfo.serialNumber;
    json["webUrl"]            = printerNetworkInfo.webUrl;
    json["isPhysicalPrinter"] = printerNetworkInfo.isPhysicalPrinter;
    json["addTime"]           = printerNetworkInfo.addTime;
    json["modifyTime"]        = printerNetworkInfo.modifyTime;
    json["lastActiveTime"]    = printerNetworkInfo.lastActiveTime;
    return json;
}

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

void PrinterManagerView::openPrinterTab(const std::string& id)
{
    auto it = mPrinterViews.find(id);
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
        if (printer.id == id) {
            printerInfo = printer;
            break;
        }
    }
    if (printerInfo.id.empty()) {
        wxLogError("Printer %s not found", id.c_str());
        return;
    }
    PrinterWebView* view = new PrinterWebView(mTabBar);
    wxString url = printerInfo.webUrl;
    view->load_url(url);
    mTabBar->AddPage(view, from_u8(printerInfo.name));
    mTabBar->SetSelection(mTabBar->GetPageCount() - 1);
    mPrinterViews[id] = view;
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
        if (cmd == "request_printer_list") {
            nlohmann::json response  = json::object();
            response["command"]     = "response_printer_list";
            response["sequence_id"] = "10001";
            response["response"]    = getPrinterList();
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_printer_model_list") {
            nlohmann::json response = json::object();
            response["command"] = "response_printer_model_list";
            response["sequence_id"] = "10002";
            response["response"] = getPrinterModelList();
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_printer_detail") {
            openPrinterTab(root["id"]);
        } else if (cmd == "request_discover_printers") {
            nlohmann::json response = json::object();
            response["command"] = "response_discover_printers";
            response["sequence_id"] = "10003";
            response["response"] = discoverPrinter();
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_bind_printer") {
            nlohmann::json response = json::object();
            response["command"] = "response_bind_printer";
            response["sequence_id"] = "10004";
            nlohmann::json printer = root["printer"];
            response["response"] = bindPrinter(printer);
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_update_printer_name") {
            nlohmann::json response = json::object();
            response["command"] = "response_update_printer_name";
            response["sequence_id"] = "10005";
            response["response"] = updatePrinterName(root["id"], root["name"]);
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_delete_printer") {
            nlohmann::json response = json::object();
            response["command"] = "response_delete_printer";
            response["sequence_id"] = "10006";
            response["response"] = deletePrinter(root["id"]);
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "test_printer_connection") {
           
        } else if (cmd == "browse_print_host") {

        } else if (cmd == "browse_ca_file") {
            nlohmann::json response = json::object();
            response["command"] = "update_ca_file";
            response["value"] = browseCAFile();
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "refresh_printers") {

        } else if (cmd == "logout_print_host") {

        } else if (cmd == "connect_physical_printer") {
  
        }
    } catch (std::exception& e) {
        wxLogMessage("Error: %s", e.what());
    }
}

bool PrinterManagerView::deletePrinter(const std::string& id)
{
    auto it = mPrinterViews.find(id);
    if (it != mPrinterViews.end()) {
        int page = mTabBar->GetPageIndex(it->second);
        if (page != wxNOT_FOUND) {
            mTabBar->DeletePage(page);
        }
        it->second->OnClose(wxCloseEvent());
        mPrinterViews.erase(it);
        mTabBar->SetSelection(0);
    }
    return PrinterManager::getInstance()->deletePrinter(id);
}
bool PrinterManagerView::updatePrinterName(const std::string& id, const std::string& name)
{
    auto it = mPrinterViews.find(id);
    if (it != mPrinterViews.end()) {
        int page = mTabBar->GetPageIndex(it->second);
        if (page != wxNOT_FOUND) {
            mTabBar->SetPageText(page, from_u8(name));
        }
    }
    return PrinterManager::getInstance()->updatePrinterName(id, name);
}
std::string PrinterManagerView::bindPrinter(const nlohmann::json& printer)
{
    PrinterNetworkInfo printerInfo = convertJsonToPrinterNetworkInfo(printer);
    return PrinterManager::getInstance()->bindPrinter(printerInfo);
}

nlohmann::json PrinterManagerView::discoverPrinter()
{
    auto printerList = PrinterManager::getInstance()->discoverPrinter();
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.machineModel + "_cover.png";
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
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.machineModel + "_cover.png";
        printer_obj["printerImg"] = imageFileToBase64DataURI(img_path);
        printer_obj["printerStatus"] = PrinterNetworkStatus::PRINTER_NETWORK_STATUS_DISCONNECTED;
        printer_obj["printProgress"] = 0;
        printer_obj["currentTicks"] = 0;
        printer_obj["totalTicks"] = 0;
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
    std::map<std::string, std::map<std::string, std::string>> vendor_printer_model_info_map;
    PresetBundle                                              bundle;

    auto [substitutions, errors] = bundle.load_system_models_from_json(ForwardCompatibilitySubstitutionRule::EnableSilent);

    // Load all vendor configurations first
    for (const auto& vendor_pair : bundle.vendors) {
        const std::string& vendor_name = vendor_pair.first;
        PresetBundle       vendor_bundle;
        vendor_bundle.load_vendor_configs_from_json((boost::filesystem::path(Slic3r::resources_dir()) / "profiles").string(), vendor_name,
                                                    PresetBundle::LoadSystem,
                                                    ForwardCompatibilitySubstitutionRule::EnableSilent, nullptr);
        for (const auto& printer : vendor_bundle.printers) {
            if (!printer.vendor) {
                continue;
            }
            std::string vendor_name   = printer.vendor->name;
            auto        printer_model = printer.config.option<ConfigOptionString>("printer_model");
            if (!printer_model) {
                continue;
            }

            std::string model_name    = printer_model->value;
            std::string host_type_str = "";

            if (PrintHost::support_device_list_management(printer.config)) {
                const auto opt       = printer.config.option<ConfigOptionEnum<PrintHostType>>("host_type");
                const auto host_type = opt != nullptr ? opt->value : htOctoPrint;
                switch (host_type) {
                case htElegooLink: host_type_str = "ElegooLink"; break;
                case htOctoPrint: host_type_str = "OctoPrint"; break;
                case htPrusaLink: host_type_str = "PrusaLink"; break;
                case htPrusaConnect: host_type_str = "PrusaConnect"; break;
                case htDuet: host_type_str = "Duet"; break;
                case htFlashAir: host_type_str = "FlashAir"; break;
                case htAstroBox: host_type_str = "AstroBox"; break;
                case htRepetier: host_type_str = "Repetier"; break;
                case htMKS: host_type_str = "MKS"; break;
                case htESP3D: host_type_str = "ESP3D"; break;
                case htCrealityPrint: host_type_str = "CrealityPrint"; break;
                case htObico: host_type_str = "Obico"; break;
                case htFlashforge: host_type_str = "Flashforge"; break;
                case htSimplyPrint: host_type_str = "SimplyPrint"; break;
                default: host_type_str = ""; break;
                }
                if (!host_type_str.empty()) {
                    vendor_printer_model_info_map[vendor_name][model_name] = host_type_str;
                }
            }
        }
    }
    
    nlohmann::json response = nlohmann::json::array();
    for (auto& vendor : vendor_printer_model_info_map) {
        nlohmann::json vendor_obj = nlohmann::json::object();
        vendor_obj["vendor"]      = vendor.first;
        vendor_obj["models"]      = nlohmann::json::array();

        for (auto& model : vendor.second) {
            nlohmann::json model_obj = nlohmann::json::object();
            model_obj["model_name"]  = model.first;
            model_obj["host_type"]   = model.second;
            vendor_obj["models"].push_back(model_obj);
        }
        response.push_back(vendor_obj);
    }

    return response;
}
} // GUI
} // Slic3r


