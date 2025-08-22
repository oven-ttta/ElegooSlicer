#include "PrintSendDialogEx.hpp"
#include "ConfigWizard.hpp"
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <string.h>
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "libslic3r_version.h"
#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include "MainFrame.hpp"
#include <boost/dll.hpp>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include <slic3r/Utils/Http.hpp>
#include <libslic3r/miniz_extension.hpp>
#include <libslic3r/Utils.hpp>
#include <wx/wx.h>
#include <wx/display.h>
#include <wx/fileconf.h>
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/base64.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <thread>
#include <memory>
#include "PrinterManager.hpp"
#include "PrinterMmsManager.hpp"
#include <slic3r/Utils/WebviewIPCManager.h>


#define HAS_MMS_HEIGHT 720
#define NO_MMS_HEIGHT 600

using namespace nlohmann;

namespace Slic3r { namespace GUI {

PrintSendDialogEx::PrintSendDialogEx(Plater*                    plater,
                                     int                        printPlateIdx,
                                     const fs::path&            path,
                                     PrintHostPostUploadActions postActions,
                                     const wxArrayString&       groups,
                                     const wxArrayString&       storagePaths,
                                     const wxArrayString&       storageNames,
                                     bool                       switchToDeviceTab)
    : PrintHostSendDialog(path, postActions, groups, storagePaths, storageNames, switchToDeviceTab)
    , mPlater(plater)
    , mPrintPlateIdx(printPlateIdx)
    , mTimeLapse(0)
    , mHeatedBedLeveling(0)
    , mBedType(BedType::btPTE)
    , m_isDestroying(false)
    , m_lifeTracker(std::make_shared<bool>(true))
    , m_asyncOperationInProgress(false)
{
    // Bind close event to handle async operations
    Bind(wxEVT_CLOSE_WINDOW, &PrintSendDialogEx::OnCloseWindow, this);
}

PrintSendDialogEx::~PrintSendDialogEx() {
    m_isDestroying = true;
    
    // Reset the life tracker to signal all async operations that this object is being destroyed
    m_lifeTracker.reset();
    
    if (mIpc) {
        delete mIpc;
        mIpc = nullptr;
    }
}

void PrintSendDialogEx::init()
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

    mIpc = new webviewIpc::WebviewIPCManager(mBrowser);
    setupIPCHandlers();
    // mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrintSendDialogEx::onScriptMessage, this);
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/print_send/index.html").make_preferred().string());
    TargetUrl = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    topsizer->Add(mBrowser, wxSizerFlags().Expand().Proportion(1));
    wxSize pSize = FromDIP(wxSize(860, NO_MMS_HEIGHT));
    SetSize(pSize);
    CenterOnParent();

    std::string uploadAndPrint = app_config->get("recent", CONFIG_KEY_UPLOADANDPRINT);
    if (!uploadAndPrint.empty())
        post_upload_action = static_cast<PrintHostPostUploadAction>(std::stoi(uploadAndPrint));
    std::string timeLapse = app_config->get("recent", CONFIG_KEY_TIMELAPSE);
    if (!timeLapse.empty())
        mTimeLapse = std::stoi(timeLapse);
    std::string heatedBedLeveling = app_config->get("recent", CONFIG_KEY_HEATEDBEDLEVELING);
    if (!heatedBedLeveling.empty())
        mHeatedBedLeveling = std::stoi(heatedBedLeveling);
    std::string bedType = app_config->get("recent", CONFIG_KEY_BEDTYPE);
    if (!bedType.empty())
        mBedType = static_cast<BedType>(std::stoi(bedType));

    std::string autoRefill = app_config->get("recent", CONFIG_KEY_AUTO_REFILL);
    if (!autoRefill.empty())
        mAutoRefill = std::stoi(autoRefill);

    wxString recent_path = from_u8(app_config->get("recent", CONFIG_KEY_PATH));
    if (recent_path.Length() > 0 && recent_path[recent_path.Length() - 1] != '/') {
        recent_path += '/';
    }
    recent_path += m_path.filename().wstring();
    txt_filename->SetValue(recent_path);
    
    // Cache the model name for use in preparePrintTask
    m_cachedModelName = recent_path.ToUTF8().data();
    if (m_cachedModelName.size() >= 6 && m_cachedModelName.compare(m_cachedModelName.size() - 6, 6, ".gcode") == 0)
        m_cachedModelName = m_cachedModelName.substr(0, m_cachedModelName.size() - 6);
    
    // if (size_t extension_start = recent_path.find_last_of('.'); extension_start != std::string::npos)
    //     m_valid_suffix = recent_path.substr(extension_start);
    // mProjectName = getCurrentProjectName();
}

std::string PrintSendDialogEx::getCurrentProjectName()
{
    wxString filename = mPlater->get_export_gcode_filename("", true, mPrintPlateIdx == PLATE_ALL_IDX ? true : false);
    if (mPrintPlateIdx == PLATE_ALL_IDX && filename.empty()) {
        filename = _L("Untitled");
    }

    if (filename.empty()) {
        filename = mPlater->get_export_gcode_filename("", true);
        if (filename.empty())
            filename = _L("Untitled");
    }

    fs::path    filenamePath(filename.c_str());
    std::string projectName = filenamePath.filename().string();
    if (from_u8(projectName).find(_L("Untitled")) != wxString::npos) {
        PartPlate* partPlate = mPlater->get_partplate_list().get_plate(mPrintPlateIdx);
        if (partPlate) {
            if (std::vector<ModelObject*> objects = partPlate->get_objects_on_this_plate(); objects.size() > 0) {
                projectName = objects[0]->name;
                for (int i = 1; i < objects.size(); i++) {
                    projectName += (" + " + objects[i]->name);
                }
            }
            if (projectName.size() > 100) {
                projectName = projectName.substr(0, 97) + "...";
            }
        }
    }
    const std::string invalidChars = "<>[]:\\/|?*\"";
    projectName.erase(std::remove_if(projectName.begin(), projectName.end(),
                                     [&invalidChars](char c) { return invalidChars.find(c) != std::string::npos; }),
                      projectName.end());
    return projectName;
}

void PrintSendDialogEx::setupIPCHandlers()
{
    if (!mIpc) return;

    // Handle request_print_task (async due to time-consuming preparePrintTask operation)
    mIpc->onRequestAsync("request_print_task", [this](const webviewIpc::IPCRequest& request,
                                                       std::function<void(const webviewIpc::IPCResult&)> sendResponse) {
        std::string printerId = request.params.value("printerId", "");
        
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;
        
        // Mark async operation as in progress to prevent window closing
        m_asyncOperationInProgress = true;
        
        // Send status to web view (optional, for UI feedback)
        if (!m_isDestroying) {
            runScript("window.onPrintTaskStarted && window.onPrintTaskStarted();");
        }
        
        // Run the print task preparation in a separate thread to avoid blocking the UI
        std::thread([life_tracker, printerId, sendResponse, this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    nlohmann::json response = this->preparePrintTask(printerId);
                    
                    // Final check before sending response
                    if (life_tracker.lock()) {
                        // Mark async operation as completed
                        m_asyncOperationInProgress = false;
                        
                        sendResponse(webviewIpc::IPCResult::success(response));
                    }
                } else {
                    // Object was destroyed, don't send response
                    return;
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    // Mark async operation as completed even on error
                    m_asyncOperationInProgress = false;
                    
                    wxLogError("Error in request_print_task: %s", e.what());
                    sendResponse(webviewIpc::IPCResult::error(std::string("Print task preparation failed: ") + e.what()));
                }
            } catch (...) {
                if (life_tracker.lock()) {
                    // Mark async operation as completed even on unknown error
                    m_asyncOperationInProgress = false;
                    
                    wxLogError("Unknown error in request_print_task");
                    sendResponse(webviewIpc::IPCResult::error("Print task preparation failed: Unknown error"));
                }
            }
        }).detach();
    });

    // Handle request_printer_list
    mIpc->onRequest("request_printer_list", [this](const webviewIpc::IPCRequest& request) {
        try {
            nlohmann::json response = getPrinterList();
            return webviewIpc::IPCResult::success(response);
        } catch (const std::exception& e) {
            wxLogError("Error in request_printer_list: %s", e.what());
            return webviewIpc::IPCResult::error("Failed to get printer list");
        }
    });

    // Handle cancel_print
    mIpc->onRequest("cancel_print", [this](const webviewIpc::IPCRequest& request) {
        try {
            onCancel();
            return webviewIpc::IPCResult::success();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error in cancel_print: " << e.what();
            return webviewIpc::IPCResult::error("Failed to cancel print");
        }
    });

    // Handle start_upload
    mIpc->onRequest("start_upload", [this](const webviewIpc::IPCRequest& request) {
        try {
            nlohmann::json printInfo = request.params.value("data", nlohmann::json::object());
            onPrint(printInfo);
            return webviewIpc::IPCResult::success();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error in start_upload: " << e.what();
            return webviewIpc::IPCResult::error("Failed to start upload");
        }
    });

    mIpc->onRequest("expand_window", [this](const webviewIpc::IPCRequest& request) {
        try {
           bool expand = request.params.value("expand", false);
            if(!expand) {
                wxGetApp().CallAfter([this]() {
                    wxSize pSize = FromDIP(wxSize(860, NO_MMS_HEIGHT));
                    SetSize(pSize);
                });
            } else {
                wxGetApp().CallAfter([this]() {
                    wxSize pSize = FromDIP(wxSize(860, HAS_MMS_HEIGHT));
                    SetSize(pSize);
                });
            }
            return webviewIpc::IPCResult::success();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error in expand_window: " << e.what();
            return webviewIpc::IPCResult::error("Failed to expand window");
        }
    });
}

void PrintSendDialogEx::onScriptMessage(wxWebViewEvent& evt)
{
    // This method is kept for compatibility but now delegates to WebviewIPCManager
    // The actual message handling is done in setupIPCHandlers()
    try {
        wxString strInput = evt.GetString();
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(trace) << "PrintSendDialogEx::onScriptMessage;Error:" << e.what();
    }
}

void PrintSendDialogEx::runScript(const wxString& javascript)
{
    if (!mBrowser)
        return;

    WebView::RunScript(mBrowser, javascript);
}

nlohmann::json PrintSendDialogEx::preparePrintTask(const std::string& printerId)
{
    nlohmann::json printTask     = json::object();
    auto           preset_bundle = wxGetApp().preset_bundle;

    const Print&   print     = mPlater->get_partplate_list().get_current_fff_print();
    const auto&    stats     = print.print_statistics();
    nlohmann::json printInfo = json::object();
    printInfo["printTime"]   = stats.estimated_normal_print_time;
    printInfo["totalWeight"] = stats.total_weight;

    int layerCount = 0;
    for (const PrintObject* object : print.objects()) {
        layerCount = std::max(layerCount, (int) object->layer_count());
    }
    printInfo["layerCount"] = layerCount;

    // Use cached model name instead of getting it from UI control
    std::string modelName = m_cachedModelName;

    printInfo["modelName"]         = modelName;
    printInfo["timeLapse"]         = mTimeLapse == 1;
    printInfo["heatedBedLeveling"] = mHeatedBedLeveling == 1;
    printInfo["switchToDeviceTab"] = m_switch_to_device_tab;
    printInfo["uploadAndPrint"]    = post_upload_action == PrintHostPostUploadAction::StartPrint;
    printInfo["autoRefill"]        = mAutoRefill == 1;
    if (mBedType == BedType::btPC)
        printInfo["bedType"] = "btPC"; // B
    else
        printInfo["bedType"] = "btPTE"; // A

    ThumbnailData& data = mPlater->get_partplate_list().get_curr_plate()->thumbnail_data;
    wxImage        image;
    if (data.is_valid()) {
        image = wxImage(data.width, data.height);
        image.InitAlpha();
        for (unsigned int r = 0; r < data.height; ++r) {
            unsigned int rr = (data.height - 1 - r) * data.width;
            for (unsigned int c = 0; c < data.width; ++c) {
                unsigned char* px = (unsigned char*) data.pixels.data() + 4 * (rr + c);
                image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                image.SetAlpha((int) c, (int) r, px[3]);
            }
        }
        image = image.Rescale(FromDIP(256), FromDIP(256));
        wxMemoryOutputStream mem;
        image.SaveFile(mem, wxBITMAP_TYPE_PNG);
        size_t         len = mem.GetSize();
        wxMemoryBuffer buffer(len);
        mem.CopyTo(buffer.GetData(), len);
        printInfo["thumbnail"] = wxBase64Encode(buffer.GetData(), len).ToStdString();
    }

    std::map<std::string, Preset*> nameToPreset;
    for (int i = 0; i < preset_bundle->filaments.size(); ++i) {
        Preset* preset             = &preset_bundle->filaments.preset(i);
        nameToPreset[preset->name] = preset;
    }

    // get printfilament list
    std::vector<PrintFilamentMmsMapping> projectFilamentList;
    mPrintFilamentList.clear();
    for (const auto& filamentName : preset_bundle->filament_presets) {
        auto it = nameToPreset.find(filamentName);
        if (it != nameToPreset.end()) {
            PrintFilamentMmsMapping filament;
            Preset*                 preset = it->second;
            std::string             filamentAlias = preset->alias;
            std::string             displayedFilamentType;
            std::string             filamentType = preset->config.get_filament_type(displayedFilamentType);
            
            // get filament density to calculate filament weight
            float                     density            = 0.0;
            const ConfigOptionFloats* filament_densities = preset->config.option<ConfigOptionFloats>("filament_density");
            if (filament_densities != nullptr) {
                density = filament_densities->values[0];
            }

            filament.filamentType    = filamentType;
            filament.filamentId      = preset->filament_id;
            filament.settingId       = preset->setting_id;
            filament.filamentName    = filamentName;
            // alias and filamentType is used to match filament in mms
            filament.filamentAlias   = filamentAlias;
            filament.filamentWeight  = 0;
            filament.filamentDensity = density;
            projectFilamentList.push_back(filament);
        }
    }
    // calculate filament weight
    auto extruders = wxGetApp().plater()->get_partplate_list().get_curr_plate()->get_used_extruders();
    for (auto i = 0; i < extruders.size(); i++) {
        int extruderIdx = extruders[i] - 1;
        if (extruderIdx < 0 || extruderIdx >= (int) projectFilamentList.size())
            continue;
        auto info = projectFilamentList[extruderIdx];
        info.index         = extruderIdx;
        auto  colour       = wxGetApp().preset_bundle->project_config.opt_string("filament_colour", (unsigned int) extruderIdx);
        info.filamentColor = colour;
        float total_weight = 0.0;

        const auto& stats = print.print_statistics();

        double model_volume_mm3 = 0.0;
        auto   model_it         = stats.filament_stats.find(extruderIdx);
        if (model_it != stats.filament_stats.end()) {
            model_volume_mm3 = model_it->second;
        }

        double wipe_tower_volume_mm3 = 0.0;
        double support_volume_mm3    = 0.0;

        auto current_plate = mPlater->get_partplate_list().get_curr_plate();
        if (current_plate && current_plate->get_slice_result()) {
            const auto& gcode_stats = current_plate->get_slice_result()->print_statistics;

            auto wipe_tower_it = gcode_stats.wipe_tower_volumes_per_extruder.find(extruderIdx);
            if (wipe_tower_it != gcode_stats.wipe_tower_volumes_per_extruder.end()) {
                wipe_tower_volume_mm3 = wipe_tower_it->second;
            }

            auto support_it = gcode_stats.support_volumes_per_extruder.find(extruderIdx);
            if (support_it != gcode_stats.support_volumes_per_extruder.end()) {
                support_volume_mm3 = support_it->second;
            }
        }
        double total_volume_mm3 = model_volume_mm3 + wipe_tower_volume_mm3 + support_volume_mm3;

        if (total_volume_mm3 > 0) {
            double raw_weight = total_volume_mm3 * info.filamentDensity * 0.001;

            if (raw_weight > 0) {
                double magnitude  = pow(10, floor(log10(raw_weight)));
                double normalized = raw_weight / magnitude;
                total_weight      = round(normalized * 100) / 100 * magnitude;
            } else {
                total_weight = 0.0;
            }
        }
        info.filamentWeight = total_weight;
        mPrintFilamentList.push_back(info);
    }
    PrinterNetworkInfo printerNetworkInfo = PrinterManager::getInstance()->getPrinterNetworkInfo(printerId);
    PrinterMmsGroup mmsGroup;

    if(!printerNetworkInfo.printerId.empty() && printerNetworkInfo.printerAttributes.capabilities.supportsMms) {
        PrinterNetworkResult<PrinterMmsGroup> res = PrinterMmsManager::getInstance()->getPrinterMmsInfo(printerId);
        if(res.isSuccess() && res.hasData()) {
            mmsGroup = res.data.value();
        }
        nlohmann::json mmsInfo  = convertPrinterMmsGroupToJson(mmsGroup);
        printInfo["mmsInfo"]    = mmsInfo;
        PrinterMmsManager::getInstance()->getFilamentMmsMapping(printerNetworkInfo, mPrintFilamentList, mmsGroup);
    } else {
        printInfo["mmsInfo"] = json::object();
    }
    
    if(mmsGroup.mmsList.size() == 0) {
        mHasMms = false;
    } else {
        mHasMms = true;
    }

    nlohmann::json filamentList = json::array();
    for (auto& filament : mPrintFilamentList) {
        filamentList.push_back(convertPrintFilamentMmsMappingToJson(filament));
    }
    printInfo["filamentList"] = filamentList;
    return printInfo;
}
nlohmann::json PrintSendDialogEx::getPrinterList()
{
    nlohmann::json                  printers    = json::array();
    std::vector<PrinterNetworkInfo> printerList = PrinterManager::getInstance()->getPrinterList();
    for (auto& printer : printerList) {
        nlohmann::json printerJson = json::object();
        printerJson                = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path      = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printerJson["printerImg"] = PrinterManager::imageFileToBase64DataURI(img_path);
        printers.push_back(printerJson);
    }
    std::string selectedPrinterId = wxGetApp().app_config->get("recent", CONFIG_KEY_SELECTED_PRINTER_ID);
    for (auto& printer : printers) {
        if (selectedPrinterId.empty() || printer["printerId"].get<std::string>() != selectedPrinterId) {
            printer["selected"] = false;
        } else {
            printer["selected"] = true;
        }
    }
    return printers;
}

void PrintSendDialogEx::onPrint(const nlohmann::json& printInfo)
{
    try {
        mSelectedPrinterId     = "";
        mTimeLapse             = printInfo["timeLapse"].get<bool>();
        mHeatedBedLeveling     = printInfo["heatedBedLeveling"].get<bool>();
        mAutoRefill            = printInfo["autoRefill"].get<bool>();
        bool uploadAndPrint    = printInfo["uploadAndPrint"].get<bool>();
        m_switch_to_device_tab = printInfo["switchToDeviceTab"].get<bool>();
        mSelectedPrinterId     = printInfo["selectedPrinterId"].get<std::string>();
        std::string bedType    = printInfo["bedType"].get<std::string>();

        if (bedType == "btPC") {
            mBedType = BedType::btPC;
        } else {
            mBedType = BedType::btPEI;
        }

        if (!mSelectedPrinterId.empty()) {
            wxGetApp().app_config->set("recent", CONFIG_KEY_SELECTED_PRINTER_ID, mSelectedPrinterId);
        } else {
            wxMessageBox("Please select a printer!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        post_upload_action = uploadAndPrint ? PrintHostPostUploadAction::StartPrint : PrintHostPostUploadAction::None;

        wxString modelName = wxString::FromUTF8(printInfo["modelName"].get<std::string>());
        if (!modelName.EndsWith(".gcode")) {
            modelName += ".gcode";
        }
        txt_filename->SetValue(modelName);

        if(uploadAndPrint && mHasMms) {
             for(auto& printFilament : mPrintFilamentList) {
                // init mappedMmsFilament
                printFilament.mappedMmsFilament = PrinterMmsTray();
                for( int i = 0; i < printInfo["filamentList"].size(); i++) {
                    nlohmann::json mappedFilament = printInfo["filamentList"][i]["mappedMmsFilament"];
                    //update printFilament with mappedFilament
                    if (printInfo["filamentList"][i]["index"] == printFilament.index) {
                        printFilament.mappedMmsFilament.trayName = mappedFilament["trayName"];
                        printFilament.mappedMmsFilament.mmsId = mappedFilament["mmsId"];
                        printFilament.mappedMmsFilament.trayId = mappedFilament["trayId"];
                        printFilament.mappedMmsFilament.filamentColor = mappedFilament["filamentColor"];
                        printFilament.mappedMmsFilament.filamentName = mappedFilament["filamentName"];
                        printFilament.mappedMmsFilament.filamentType = mappedFilament["filamentType"];
                        break;
                    }
                }
             }
             for (auto& printFilament : mPrintFilamentList) {
                 if (printFilament.mappedMmsFilament.trayName.empty() || printFilament.mappedMmsFilament.mmsId.empty() ||
                     printFilament.mappedMmsFilament.trayId.empty() || printFilament.mappedMmsFilament.filamentColor.empty() ||
                     printFilament.mappedMmsFilament.filamentName.empty() || printFilament.mappedMmsFilament.filamentType.empty()) {
                        wxMessageBox("Some filaments are not mapped!", "Error", wxOK | wxICON_ERROR);
                        EndModal(wxID_CANCEL);
                        return;
                 }
             }
             PrinterMmsManager::getInstance()->saveFilamentMmsMapping(mPrintFilamentList);
        }
        EndModal(wxID_OK);
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Print Error: " << e.what();
        EndModal(wxID_CANCEL);
    }
}

void PrintSendDialogEx::onCancel() { EndModal(wxID_CANCEL); }

void PrintSendDialogEx::EndModal(int ret)
{
    if (ret == wxID_OK) {
        AppConfig* app_config = wxGetApp().app_config;
        app_config->set("recent", CONFIG_KEY_UPLOADANDPRINT, std::to_string(static_cast<int>(post_upload_action)));
        app_config->set("recent", CONFIG_KEY_TIMELAPSE, std::to_string(mTimeLapse));
        app_config->set("recent", CONFIG_KEY_HEATEDBEDLEVELING, std::to_string(mHeatedBedLeveling));
        app_config->set("recent", CONFIG_KEY_BEDTYPE, std::to_string(static_cast<int>(mBedType)));
        app_config->set("recent", CONFIG_KEY_AUTO_REFILL, std::to_string(mAutoRefill));
    }

    PrintHostSendDialog::EndModal(ret);
}

std::map<std::string, std::string> PrintSendDialogEx::extendedInfo() const
{
    nlohmann::json filamentList = json::array();
    if(mHasMms) {
        for (auto& filament : mPrintFilamentList) {
            filamentList.push_back(convertPrintFilamentMmsMappingToJson(filament));
        }
    }

    return {{"bedType", std::to_string(mBedType)},
            {"timeLapse", mTimeLapse ? "true" : "false"},
            {"heatedBedLeveling", mHeatedBedLeveling ? "true" : "false"},
            {"autoRefill", mAutoRefill ? "true" : "false"},
            {"selectedPrinterId", mSelectedPrinterId},
            {"filamentAmsMapping", filamentList.dump()}};
}

void PrintSendDialogEx::OnCloseWindow(wxCloseEvent& event)
{
    // If async operation is in progress, prevent closing
    if (m_asyncOperationInProgress && !m_isDestroying) {
        // Show a message to user that operation is in progress
        // wxMessageBox(_L("Print task preparation is in progress. Please wait..."), 
        //              _L("Operation in Progress"), wxOK | wxICON_INFORMATION);
        event.Veto(); // Prevent the window from closing
        return;
    }
    
    // Allow normal close behavior
    event.Skip();
}

}} // namespace Slic3r::GUI
