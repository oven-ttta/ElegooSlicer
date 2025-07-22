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

#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <fstream>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "slic3r/Utils/PrinterNetworkManager.hpp"

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

#define FIRST_TAB_NAME _L("Connected Printer")


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

namespace Slic3r {
namespace GUI {

PrinterManager::PrinterManager(wxWindow *parent)
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
    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/printer.html").make_preferred().string());
    TargetUrl = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);
    mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterManager::onScriptMessage, this);

    loadPrinterList();
    
    // for (auto& printer : mPrinterList) {
    //     openPrinterTab(printer.second.id);
    // }
    mTabBar->SetSelection(0);
    Bind(wxEVT_CLOSE_WINDOW, &PrinterManager::onClose, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PrinterManager::onClosePrinterTab, this);
}

PrinterManager::~PrinterManager() {

}

void PrinterManager::openPrinterTab(const std::string& id)
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
    auto printerInfo = mPrinterList[id];
    PrinterWebView* view = new PrinterWebView(mTabBar);
    wxString url = "http://" + printerInfo.ip;
    view->load_url(url);
    mTabBar->AddPage(view, printerInfo.ip);
    mTabBar->SetSelection(mTabBar->GetPageCount() - 1);
    mPrinterViews[id] = view;
    Layout();
}

void PrinterManager::loadPrinterList()
{
    fs::path printerListPath = boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_list";
    if (!boost::filesystem::exists(printerListPath)) {
        mPrinterList.clear();
        return;
    }
    std::ifstream ifs(printerListPath.string(), std::ios::binary);
    if (!ifs.is_open()) {
        wxLogError("open printer list file failed: %s", printerListPath.string().c_str());
        mPrinterList.clear();
        return;
    }
    try {
        cereal::BinaryInputArchive iarchive(ifs);
        iarchive(mPrinterList);
    } catch (const std::exception& e) {
        wxLogError("printer list deserialize failed: %s", e.what());
        mPrinterList.clear();
    }
}


void PrinterManager::savePrinterList()
{
    fs::path printerListPath = boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_list";
    std::ofstream ofs(printerListPath.string(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(ofs);
    oarchive(mPrinterList);
}

void PrinterManager::onScriptMessage(wxWebViewEvent& event)
{
    wxString message = event.GetString();
    try {
        boost::property_tree::ptree root;
        std::stringstream           json_stream(message.ToStdString());
        boost::property_tree::read_json(json_stream, root);
        std::string cmd = root.get<std::string>("command");
        if (cmd == "request_printer_list") {
            nlohmann::json response           = json::object();
            response["command"]     = "response_printer_list";
            response["sequence_id"] = "10001";
            response["response"]    = getPrinterList();
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_printer_detail") {
            openPrinterTab(root.get<std::string>("id"));
        } else if (cmd == "request_discover_printers") {
            nlohmann::json response = json::object();
            response["command"] = "response_discover_printers";
            response["sequence_id"] = "10002";
            response["response"] = discoverPrinter();
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_bind_printers") {
            nlohmann::json printers = nlohmann::json::array();
            for (const auto& printer : root.get_child("printers")) {
                nlohmann::json printer_obj = nlohmann::json::object();
                for (const auto& field : printer.second) {
                    printer_obj[field.first] = field.second.data();
                }
                printers.push_back(printer_obj);
            }
            nlohmann::json response = json::object();
            response["command"] = "response_bind_printers";
            response["sequence_id"] = "10003";
            response["response"] = bindPrinters(printers);
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_update_printer_name") {
            nlohmann::json response = json::object();
            response["command"] = "response_update_printer_name";
            response["sequence_id"] = "10004";
            response["response"] = updatePrinterName(root.get<std::string>("id"), root.get<std::string>("name"));
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (cmd == "request_delete_printer") {
            nlohmann::json response = json::object();
            response["command"] = "response_delete_printer";
            response["sequence_id"] = "10005";
            response["response"] = deletePrinter(root.get<std::string>("id"));
            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        }
    } catch (std::exception& e) {
        wxLogMessage("Error: %s", e.what());
    }
}

bool PrinterManager::deletePrinter(const std::string& id)
{
    auto it = mPrinterList.find(id);
    if (it != mPrinterList.end()) {
        mPrinterList.erase(it);
        savePrinterList();
        return true;
    }
    return false;
}
bool PrinterManager::updatePrinterName(const std::string& id, const std::string& name)
{
    auto it = mPrinterList.find(id);
    if (it != mPrinterList.end()) {
        it->second.name = name;
        savePrinterList();
        return true;
    }
    return false;
}
nlohmann::json PrinterManager::bindPrinters(nlohmann::json& printers)
{
    nlohmann::json response = json::array();
    std::string xx = printers.dump(-1, ' ', true);
    for (auto& printer : printers) {
        PrinterInfo printerInfo;
        printerInfo.id = printer["id"];
        printerInfo.name = printer["name"];
        printerInfo.ip = printer["ip"];
        printerInfo.port = printer["port"];
        printerInfo.vendor = printer["vendor"];
        printerInfo.machineName = printer["machineName"];
        printerInfo.machineModel = printer["machineModel"];
        printerInfo.protocolVersion = printer["protocolVersion"];
        printerInfo.firmwareVersion = printer["firmwareVersion"];
        printerInfo.deviceId = printer["deviceId"];
        printerInfo.deviceType = printer["deviceType"];
        printerInfo.connectionUrl = printer["connectionUrl"];
        printerInfo.serialNumber = printer["serialNumber"];
        printerInfo.webUrl = printer["webUrl"];
        printerInfo.isPhysicalPrinter = printer["isPhysicalPrinter"].get<std::string>() == "true";
        // for(auto& printerInfo : mPrinterList) {
        //     if(printerInfo.second.ip == printer["ip"] || printerInfo.second.deviceId == printer["deviceId"]) {
        //         continue;
        //     }
        // }
        mPrinterList[printerInfo.id] = printerInfo;
        response.push_back(printer);
    }
    savePrinterList();
    return response;
}
VendorProfile getMachineProfile(const std::string& vendorName, const std::string& machineModel, VendorProfile::PrinterModel& printerModel)
{
    std::string profile_vendor_name;    
    VendorProfile machineProfile;
    PresetBundle bundle;
    auto [substitutions, errors] = bundle.load_system_models_from_json(ForwardCompatibilitySubstitutionRule::EnableSilent);
    for (const auto& vendor : bundle.vendors) {
        const auto& vendor_profile = vendor.second;
        if (boost::to_upper_copy(vendor_profile.name) == boost::to_upper_copy(vendorName)) {
            for (const auto& model : vendor_profile.models) {
                if (boost::to_upper_copy(model.name).find(boost::to_upper_copy(machineModel)) != std::string::npos) {
                    machineProfile = vendor_profile;
                    printerModel = model;
                    break;
                }
            }
            break;
        }
    }  
    return machineProfile;
}

nlohmann::json PrinterManager::discoverPrinter()
{
    boost::filesystem::path resources_path(Slic3r::resources_dir());
    nlohmann::json printers = json::array();
 
    std::vector<PrinterInfo> discoverPrinterList = PrinterNetworkManager::getInstance()->discoverPrinters();
    for (auto& discoverPrinter : discoverPrinterList) {
        for (auto& p : mPrinterList) {
            if (p.second.ip == discoverPrinter.ip || p.second.deviceId == discoverPrinter.deviceId) {
                continue;
            }
        }
        nlohmann::json printer = json::object();
        printer["id"] = boost::uuids::to_string(boost::uuids::random_generator{}());
        printer["name"] = discoverPrinter.name;
        printer["ip"] = discoverPrinter.ip;
        printer["port"] = discoverPrinter.port;
        VendorProfile::PrinterModel printerModel;
        VendorProfile machineProfile = getMachineProfile(discoverPrinter.vendor, discoverPrinter.machineModel, printerModel);
        printer["vendor"] = machineProfile.name;
        printer["machineName"] = printerModel.name;
        printer["machineModel"] = printerModel.name;
        std::string img_path = resources_path.string() + "/profiles/" + machineProfile.name + "/" + printerModel.name + "_cover.png";
        printer["printerImg"] = imageFileToBase64DataURI(img_path);
        printer["protocolVersion"] = discoverPrinter.protocolVersion;
        printer["firmwareVersion"] = discoverPrinter.firmwareVersion;
        printer["connectionUrl"] = discoverPrinter.connectionUrl;
        printer["deviceId"] = discoverPrinter.deviceId;
        printer["deviceType"] = discoverPrinter.deviceType;
        printer["serialNumber"] = discoverPrinter.serialNumber;
        printer["webUrl"] = discoverPrinter.webUrl;
        printer["isPhysicalPrinter"] = false;
        printer["connectionUrl"] = discoverPrinter.connectionUrl;
        printers.push_back(printer);
    }

    return printers;

}
nlohmann::json PrinterManager::getPrinterList()
{  
    boost::filesystem::path resources_path(Slic3r::resources_dir());
    nlohmann::json printers = json::array();
    for (auto& printerInfo : mPrinterList) {
        nlohmann::json printer = json::object();
        printer["id"] = printerInfo.second.id;
        printer["name"] = printerInfo.second.name;
        printer["ip"] = printerInfo.second.ip;
        printer["port"] = printerInfo.second.port;
        printer["vendor"] = printerInfo.second.vendor;
        printer["machineName"] = printerInfo.second.machineName;
        printer["machineModel"] = printerInfo.second.machineModel;
        std::string img_path = resources_path.string() + "/profiles/" + printerInfo.second.vendor + "/" + printerInfo.second.machineModel + "_cover.png";
        printer["printerImg"] = imageFileToBase64DataURI(img_path);
        printer["protocolVersion"] = printerInfo.second.protocolVersion;
        printer["firmwareVersion"] = printerInfo.second.firmwareVersion;
        printer["printerStatus"] = DeviceStatus::DEVICE_STATUS_OFFLINE;
        printer["printProgress"] = 0;
        printer["currentTicks"] = 0;
        printer["totalTicks"] = 0;
        printer["isPhysicalPrinter"] = printerInfo.second.isPhysicalPrinter;
        printer["deviceId"] = printerInfo.second.deviceId;
        printer["deviceType"] = printerInfo.second.deviceType;
        printer["connectionUrl"] = printerInfo.second.connectionUrl;
        printer["serialNumber"] = printerInfo.second.serialNumber;
        printer["webUrl"] = printerInfo.second.webUrl;
        printers.push_back(printer);
    }
    return printers;
}

void PrinterManager::notifyPrintInfo(PrinterInfo printerInfo, int printerStatus, int printProgress, int currentTicks, int totalTicks)
{
    nlohmann::json response = json::object();
    response["command"] = "notify_print_info";
    response["msg"] = printerInfo.id;
    response["printerStatus"] = printerStatus;
    response["printProgress"] = printProgress;
    response["currentTicks"] = currentTicks;
    response["totalTicks"] = totalTicks;
    wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));
    wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
}

void PrinterManager::runScript(const wxString& javascript)
{
    if (!mBrowser)
        return;
    WebView::RunScript(mBrowser, javascript);
}

void PrinterManager::onClose(wxCloseEvent& evt)
{
    this->Hide();
}

void PrinterManager::onClosePrinterTab(wxAuiNotebookEvent& event)
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
bool PrinterManager::upload(PrintHostUpload       upload_data,
                            PrintHost::ProgressFn prorgess_fn,
                            PrintHost::ErrorFn    error_fn,
                            PrintHost::InfoFn     info_fn)
{
    std::string printerId = upload_data.extended_info["selectedPrinterId"];
    if (mPrinterList.find(printerId) == mPrinterList.end()) {
        return false;
    }
    PrinterInfo printerInfo = mPrinterList[printerId];

    if (!PrinterNetworkManager::getInstance()->isPrinterConnected(printerInfo)) {
        if (!PrinterNetworkManager::getInstance()->connectToPrinter(printerInfo)) {
            error_fn(wxString::FromUTF8("connect to printer failed"));
            return false;
        }
    }

    PrinterNetworkParams params;
    params.uploadData = upload_data;
    params.progressFn = prorgess_fn;
    params.errorFn    = error_fn;
    params.infoFn     = info_fn;
    if (PrinterNetworkManager::getInstance()->sendPrintFile(printerInfo, params)) {
        if (upload_data.post_action == PrintHostPostUploadAction::StartPrint) {
            PrinterNetworkManager::getInstance()->sendPrintTask(printerInfo, params);
        }
        return true;
    }
    return false;
}
} // GUI
} // Slic3r
