#include "PrinterMmsSyncView.hpp"
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
    , m_isDestroying(false)
    , m_lifeTracker(std::make_shared<bool>(true))
    , m_asyncOperationInProgress(false)
{
    const AppConfig* app_config = wxGetApp().app_config;

    auto preset_bundle = wxGetApp().preset_bundle;
    auto model_id      = preset_bundle->printers.get_edited_preset().get_printer_type(preset_bundle);

    // Bind close event to handle async operations
    Bind(wxEVT_CLOSE_WINDOW, &PrinterMmsSyncView::OnCloseWindow, this);
    
    // Bind ESC key hook to disable ESC key closing the dialog
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE) {
            // Do nothing - disable ESC key closing the dialog
            return;
        }
        e.Skip();
    });

    SetIcon(wxNullIcon);
    // DestroyChildren();
    mBrowser = WebView::CreateWebView(this, "");
    if (mBrowser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    mIpc = new webviewIpc::WebviewIPCManager(mBrowser);
    setupIPCHandlers();
    // mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterMmsSyncView::onScriptMessage, this);
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

PrinterMmsSyncView::~PrinterMmsSyncView() {
    m_isDestroying = true;
    
    // Reset the life tracker to signal all async operations that this object is being destroyed
    m_lifeTracker.reset();
    
    if (mIpc) {
        delete mIpc;
        mIpc = nullptr;
    }
}

void PrinterMmsSyncView::setupIPCHandlers()
{
    if (!mIpc) return;

    // Handle getPrinterList
    mIpc->onRequest("getPrinterList", [this](const webviewIpc::IPCRequest& request) {
        try {
            return getPrinterList();
        } catch (const std::exception& e) {
            wxLogError("Error in getPrinterList: %s", e.what());
            return webviewIpc::IPCResult::error("Failed to get printer list");
        }
    });

    // Handle getPrinterFilamentInfo (async due to potentially time-consuming operations)
    mIpc->onRequestAsync("getPrinterFilamentInfo", [this](const webviewIpc::IPCRequest& request,
                                                          std::function<void(const webviewIpc::IPCResult&)> sendResponse) {
        nlohmann::json params = request.params;
        
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;
        
        // Mark async operation as in progress to prevent window closing
        m_asyncOperationInProgress = true;
        
        // Run the filament info retrieval in a separate thread to avoid blocking the UI
        std::thread([life_tracker, params, sendResponse, this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    webviewIpc::IPCResult response = this->getPrinterFilamentInfo(params);
                    
                    // Final check before sending response
                    if (life_tracker.lock()) {
                        // Mark async operation as completed
                        m_asyncOperationInProgress = false;
                        
                        sendResponse(response);
                    }
                } else {
                    // Object was destroyed, don't send response
                    return;
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    // Mark async operation as completed even on error
                    m_asyncOperationInProgress = false;
                    
                    wxLogError("Error in getPrinterFilamentInfo: %s", e.what());
                    sendResponse(webviewIpc::IPCResult::error(std::string("Failed to get printer filament info: ") + e.what()));
                }
            } catch (...) {
                if (life_tracker.lock()) {
                    // Mark async operation as completed even on unknown error
                    m_asyncOperationInProgress = false;
                    
                    wxLogError("Unknown error in getPrinterFilamentInfo");
                    sendResponse(webviewIpc::IPCResult::error("Failed to get printer filament info: Unknown error"));
                }
            }
        }).detach();
    });

    // Handle syncMmsFilament
    mIpc->onEvent("syncMmsFilament", [this](const webviewIpc::IPCEvent& event) {
        try {
            syncMmsFilament(event.data);
            EndModal(wxID_OK);
        } catch (const std::exception& e) {
            wxLogError("Error in syncMmsFilament: %s", e.what());
        }
    });

    // Handle closeDialog
    mIpc->onEvent("closeDialog", [this](const webviewIpc::IPCEvent& event) {
        try {
            EndModal(wxID_CANCEL);
        } catch (const std::exception& e) {
            wxLogError("Error in closeDialog: %s", e.what());
        }
    });
}

void PrinterMmsSyncView::EndModal(int ret)
{
    MsgDialog::EndModal(ret);
}

webviewIpc::IPCResult PrinterMmsSyncView::getPrinterList()
{
    webviewIpc::IPCResult result;
    std::string selectedPrinterId = wxGetApp().app_config->get("recent", CONFIG_KEY_SELECTED_PRINTER_ID);
    auto printerList = PrinterManager::getInstance()->getPrinterList();
    nlohmann::json printerArray = json::array();
    for (auto& printer : printerList) {
        if(!printer.printerAttributes.capabilities.supportsMms) {
            continue;
        }
        nlohmann::json printerObj = nlohmann::json::object();
        printerObj = convertPrinterNetworkInfoToJson(printer);
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

    std::string printerModel = "";
    auto cfg = wxGetApp().preset_bundle->printers.get_edited_preset().config;
    auto printerModelValue = cfg.option<ConfigOptionString>("printer_model");
    if (printerModelValue) {
        printerModel = printerModelValue->value;
    }
    PrinterNetworkInfo selectedPrinter = PrinterManager::getInstance()->getSelectedPrinter(printerModel, selectedPrinterId);
    for(auto& printer : printerArray) {
        if(printer["printerId"].get<std::string>() == selectedPrinter.printerId) {
            printer["selected"] = true;
        } else {
            printer["selected"] = false;
        }
    }
    result.data = printerArray;
    result.code = 0;
    result.message = getErrorMessage(PrinterNetworkErrorCode::SUCCESS);
    return result;
}

webviewIpc::IPCResult PrinterMmsSyncView::getPrinterFilamentInfo(const nlohmann::json& params)
{
    webviewIpc::IPCResult result;
    nlohmann::json printerFilamentInfo = nlohmann::json::object();
    std::string printerId = params["printerId"];
    auto mmsGroupResult = PrinterMmsManager::getInstance()->getPrinterMmsInfo(printerId);
    if(!mmsGroupResult.isSuccess() || !mmsGroupResult.hasData()) {
        result.code = static_cast<int>(PrinterNetworkErrorCode::PRINTER_MMS_NOT_CONNECTED);
        result.message = mmsGroupResult.message;
        return result;
    }
    PrinterMmsGroup mmsGroup = mmsGroupResult.data.value();
    nlohmann::json mmsInfo = convertPrinterMmsGroupToJson(mmsGroup);
    printerFilamentInfo["mmsInfo"] = mmsInfo;

    auto  preset_bundle = wxGetApp().preset_bundle;
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
    PrinterNetworkInfo printerNetworkInfo = PrinterManager::getInstance()->getPrinterNetworkInfo(printerId);
    if(!printerNetworkInfo.printerId.empty()) {
        PrinterMmsManager::getInstance()->getFilamentMmsMapping(printerNetworkInfo, printFilamentList, mmsGroup);
        for (auto& printFilament : printFilamentList) {
            printFilamentArray.push_back(convertPrintFilamentMmsMappingToJson(printFilament));
        }
    }
    printerFilamentInfo["printFilamentList"] = printFilamentArray;
    result.data = printerFilamentInfo;
    result.code = 0;
    result.message = getErrorMessage(PrinterNetworkErrorCode::SUCCESS);
    return result;
}

webviewIpc::IPCResult PrinterMmsSyncView::syncMmsFilament(const nlohmann::json& params)
{
    webviewIpc::IPCResult result;
    nlohmann::json mmsInfo = params["mmsInfo"];
    nlohmann::json printFilamentList = params["printFilamentList"];
    nlohmann::json printer = params["printer"];

    mMmsGroup  = convertJsonToPrinterMmsGroup(mmsInfo);

    std::string selectedPrinterId = printer["printerId"].get<std::string>();
    if(!selectedPrinterId.empty()) {
        wxGetApp().app_config->set("recent", CONFIG_KEY_SELECTED_PRINTER_ID, selectedPrinterId);
    }
    result.code = 0;
    result.message = getErrorMessage(PrinterNetworkErrorCode::SUCCESS);
    return result;
}

PrinterMmsGroup PrinterMmsSyncView::getSyncedMmsGroup()
{
    return mMmsGroup;
}

void PrinterMmsSyncView::onShow()
{
    // Refresh the web view when shown
    if (mBrowser) {
        mBrowser->Refresh();
    }
}

int PrinterMmsSyncView::ShowModal()
{
    onShow();
    return MsgDialog::ShowModal();
}

void PrinterMmsSyncView::OnCloseWindow(wxCloseEvent& event)
{
    // If async operation is in progress, prevent closing
    if (m_asyncOperationInProgress && !m_isDestroying) {
        // Show a message to user that operation is in progress
        // wxMessageBox(_L("Filament info retrieval is in progress. Please wait..."), 
        //              _L("Operation in Progress"), wxOK | wxICON_INFORMATION);
        event.Veto(); // Prevent the window from closing
        return;
    }
    
    // Allow normal close behavior
    event.Skip();
}

}} // namespace Slic3r::GUI
