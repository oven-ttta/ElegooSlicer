#include "PrinterMmsSyncView.hpp"
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "I18N.hpp"
#include "MainFrame.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/Utils/PrinterManager.hpp"
#include "slic3r/Utils/PrinterMmsManager.hpp"


namespace Slic3r { namespace GUI {

PrinterMmsSyncView::PrinterMmsSyncView(wxWindow* parent) : MsgDialog(static_cast<wxWindow*>(wxGetApp().mainframe), _L("Printer MMS Sync"), _L(""), 0)
{
    const AppConfig* app_config = wxGetApp().app_config;

    auto preset_bundle = wxGetApp().preset_bundle;
    auto model_id      = preset_bundle->printers.get_edited_preset().get_printer_type(preset_bundle);

    SetIcon(wxNullIcon);
    // DestroyChildren();
    mBrowser = WebView::CreateWebView(this, "");
    if (mBrowser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterMmsSyncView::onScriptMessage, this);
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/filament_sync/index.html").make_preferred().string());
    TargetUrl          = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    topsizer->Add(mBrowser, wxSizerFlags().Expand().Proportion(1));
    wxSize pSize = FromDIP(wxSize(800, 700));
    SetSize(pSize);
    CenterOnParent();
}

PrinterMmsSyncView::~PrinterMmsSyncView() {}

void PrinterMmsSyncView::runScript(const wxString& javascript)
{
    if (!mBrowser)
        return;
    WebView::RunScript(mBrowser, javascript);
}

void PrinterMmsSyncView::onScriptMessage(wxWebViewEvent& event)
{
    wxString message = event.GetString();
    try {
        nlohmann::json root = nlohmann::json::parse(message.ToUTF8().data());
        std::string    method  = root["method"];
        std::string    id  = root["id"];
        nlohmann::json params = root["params"];

        handleCommand(method, id, params);

    } catch (std::exception& e) {
        wxLogMessage("Error: %s", e.what());
    }
}

void PrinterMmsSyncView::handleCommand(const std::string& method, const std::string& id, const nlohmann::json& params)
{
    if (method == "getPrinterList") {
        sendResponse("getPrinterList", id, getPrinterList());
    } else if (method == "getPrinterFilamentInfo") {
        sendResponse("getPrinterFilamentInfo", id, getPrinterFilamentInfo(params));
    } else if (method == "closeDialog") {
        this->Hide();
    } else {
        wxLogWarning("Unknown command: %s", method);
    }
}

nlohmann::json PrinterMmsSyncView::getPrinterList()
{
    std::string selectedPrinterId = wxGetApp().app_config->get("recent", CONFIG_KEY_SELECTED_PRINTER_ID);
    auto printerList = PrinterManager::getInstance()->getPrinterList();
    nlohmann::json printerArray = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printerObj = nlohmann::json::object();
        printerObj = PrinterManager::convertPrinterNetworkInfoToJson(printer);
        if (selectedPrinterId.empty() || printer.printerId != selectedPrinterId) {
            printerObj["selected"] = false;
        } else {
            printerObj["selected"] = true;
        }
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printerObj["printerImg"] = PrinterManager::imageFileToBase64DataURI(img_path);
        printerArray.push_back(printerObj);
    }
    nlohmann::json response = {
        {"code", 0},
        {"message", "success"},
        {"data", printerArray}
    };
    return response;
}

nlohmann::json PrinterMmsSyncView::getPrinterFilamentInfo(const nlohmann::json& params)
{
    nlohmann::json printerFilamentInfo = nlohmann::json::object();
    std::string printerId = params["printerId"];
    PrinterMmsGroup mmsGroup = PrinterMmsManager::getInstance()->getPrinterMmsInfo(printerId);
    nlohmann::json mmsInfo = PrinterMmsManager::convertPrinterMmsGroupToJson(mmsGroup);
    printerFilamentInfo["mmsInfo"] = mmsInfo;

    auto           preset_bundle = wxGetApp().preset_bundle;
    std::vector<PrintFilamentMmsMapping> printFilamentList;
    nlohmann::json printFilamentArray     = json::array();

    std::map<std::string, Preset*> nameToPreset;
    for (int i = 0; i < preset_bundle->filaments.size(); ++i) {
        Preset* preset             = &preset_bundle->filaments.preset(i);
        nameToPreset[preset->name] = preset;
    }

    std::vector<PrintFilamentMmsMapping> allFilamentList;
    for (const auto& filamentName : preset_bundle->filament_presets) {
        auto it = nameToPreset.find(filamentName);
        if (it != nameToPreset.end()) {
            PrintFilamentMmsMapping filament;
            Preset*                 preset = it->second;
            std::string             displayFilamentType;
            std::string             filamentType = preset->config.get_filament_type(displayFilamentType);
            std::string displayFilamentName = filamentName;
            size_t      pos                 = displayFilamentName.find('@');
            if (pos != std::string::npos) {
                displayFilamentName = displayFilamentName.substr(0, pos);
            }
            filament.filamentType    = filamentType;
            filament.filamentId      = preset->filament_id;
            filament.filamentName    = displayFilamentName;
            allFilamentList.push_back(filament);
        }
    }
    auto extruders = wxGetApp().plater()->get_partplate_list().get_curr_plate()->get_used_extruders();
    for (auto i = 0; i < extruders.size(); i++) {
        int extruderIdx = extruders[i] - 1;
        if (extruderIdx < 0 || extruderIdx >= (int) allFilamentList.size())
            continue;
        auto info = allFilamentList[extruderIdx];
        printFilamentList.push_back(info);
    }
    PrinterMmsManager::getInstance()->getFilamentMmsMapping(printFilamentList, mmsGroup);
    for (auto& printFilament : printFilamentList) {
        printFilamentArray.push_back(PrinterMmsManager::convertPrintFilamentMmsMappingToJson(printFilament));
    }
    printerFilamentInfo["printFilamentList"] = printFilamentArray;
    nlohmann::json response = {
        {"code", 0},
        {"message", "success"},
        {"data", printerFilamentInfo}
    };
    return response;
}
void PrinterMmsSyncView::sendResponse(const std::string& method, const std::string& id, const nlohmann::json& response)
{
    nlohmann::json jsonResponse = {
        {"id", id},
        {"method", method},
        {"type", "response"},
        {"code", response["code"]},
        {"message", response["message"]},
        {"data", response["data"]}
    };
        
    wxString strJS = wxString::Format("HandleStudio(%s)", jsonResponse.dump(-1, ' ', true));
    wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
}

void PrinterMmsSyncView::onShow()
{
    // Refresh the web view when shown
    if (mBrowser) {
        mBrowser->Refresh();
    }
}

}} // namespace Slic3r::GUI
