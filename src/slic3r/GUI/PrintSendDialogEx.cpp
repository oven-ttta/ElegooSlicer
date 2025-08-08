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
#include "PrinterManager.hpp"

static const char* CONFIG_KEY_PATH    = "printhost_path";
static const char* CONFIG_KEY_GROUP   = "printhost_group";
static const char* CONFIG_KEY_STORAGE = "printhost_storage";

static const char* CONFIG_KEY_UPLOADANDPRINT      = "printsend_upload_and_print";
static const char* CONFIG_KEY_TIMELAPSE           = "printsend_timelapse";
static const char* CONFIG_KEY_HEATEDBEDLEVELING   = "printsend_heated_bed_leveling";
static const char* CONFIG_KEY_BEDTYPE             = "printsend_bed_type";
static const char* CONFIG_KEY_AUTO_REFILL         = "printsend_auto_refill";
static const char* CONFIG_KEY_SELECTED_PRINTER_ID = "printsend_selected_printer_id";

using namespace nlohmann;

namespace Slic3r { namespace GUI {

struct ColorRange
{
    int         r1, g1, b1;
    int         r2, g2, b2;
    std::string name;
    std::string hex_right;
};

static void hexToRgb(const std::string& hex, int& r, int& g, int& b)
{
    std::string h = hex;
    if (h[0] == '#')
        h = h.substr(1);
    if (h.length() == 6) {
        r = std::stoi(h.substr(0, 2), nullptr, 16);
        g = std::stoi(h.substr(2, 2), nullptr, 16);
        b = std::stoi(h.substr(4, 2), nullptr, 16);
    } else {
        r = g = b = 0;
    }
}

static const std::vector<ColorRange> colorRanges = {
    {230, 230, 230, 255, 255, 255, "white", "#FFFFFF"},             // #E6E6E6 ~ #FFFFFF
    {225, 202, 26, 255, 250, 246, "yellow", "#FFFAF6"},             // #E1CA1A ~ #FFFAF6
    {179, 204, 82, 255, 254, 162, "light_yellow_green", "#FFFEA2"}, // #B3CC52 ~ #FFFEA2
    {0, 164, 18, 49, 244, 98, "green", "#31F462"},                  // #00A412 ~ #31F462
    {0, 79, 31, 47, 159, 111, "dark_green", "#2F9F6F"},             // #004F1F ~ #2F9F6F
    {0, 58, 91, 51, 138, 171, "blue_green", "#338AAB"},             // #003A5B ~ #338AAB
    {0, 186, 120, 51, 255, 200, "cyan_green", "#33FFC8"},           // #00BA78 ~ #33FFC8
    {76, 177, 203, 156, 255, 254, "sky_blue", "#9CFFFE"},           // #4CB1CB ~ #9CFFFE
    {32, 127, 210, 112, 207, 255, "light_blue", "#70CFFF"},         // #207FD2 ~ #70CFFF
    {0, 40, 183, 80, 120, 255, "dark_blue", "#5078FF"},             // #0028B7 ~ #5078FF
    {27, 8, 97, 107, 88, 177, "purple", "#6B58B1"},                 // #1B0861 ~ #6B58B1
    {120, 19, 207, 200, 99, 255, "light_purple", "#C863FF"},        // #7813CF ~ #C863FF
    {203, 7, 208, 255, 87, 255, "pink_purple", "#FF57FF"},          // #CB07D0 ~ #FF57FF
    {172, 137, 181, 252, 217, 255, "light_pink_purple", "#FCD9FF"}, // #AC89B5 ~ #FCD9FF
    {209, 53, 79, 255, 133, 159, "pink", "#FF859F"},                // #D1354F ~ #FF859F
    {215, 0, 0, 255, 74, 73, "red", "#FF4A49"},                     // #D70000 ~ #FF4A49
    {84, 36, 0, 164, 116, 40, "brown", "#A47428"},                  // #542400 ~ #A47428
    {208, 101, 14, 255, 181, 94, "orange", "#FFB55E"},              // #D0650E ~ #FFB55E
    {212, 195, 175, 255, 255, 254, "beige", "#FFFFFF"},             // #D4C3AF ~ #FFFFFF
    {170, 157, 123, 250, 237, 203, "light_brown", "#FAEDCB"},       // #AA9D7B ~ #FAEDCB
    {135, 80, 10, 215, 160, 90, "dark_brown", "#D7A05A"},           // #87500A ~ #D7A05A
    {89, 89, 89, 153, 153, 153, "dark_gray", "#999999"},            // #595959 ~ #999999
    {153, 153, 153, 230, 230, 230, "light_gray", "#E6E6E6"},        // #999999 ~ #E6E6E6
    {0, 0, 0, 89, 89, 89, "black", "#595959"}                       // #000000 ~ #595959
};

static std::string getStandardColor(const std::string& hex)
{
    int r, g, b;
    hexToRgb(hex, r, g, b);
    for (const auto& range : colorRanges) {
        if (r >= range.r1 && r <= range.r2 && g >= range.g1 && g <= range.g2 && b >= range.b1 && b <= range.b2) {
            return range.hex_right;
        }
    }
    return "#000000";
}

PrintSendDialogEx::PrintSendDialogEx(Plater*                                  plater,
                                 int                                      printPlateIdx,
                                 const fs::path&                          path,
                                 PrintHostPostUploadActions               postActions,
                                 const wxArrayString&                     groups,
                                 const wxArrayString&                     storagePaths,
                                 const wxArrayString&                     storageNames,
                                 bool                                     switchToDeviceTab,
                                 const std::map<int, DynamicPrintConfig>& filamentAmsList)
    : PrintHostSendDialog(path, postActions, groups, storagePaths, storageNames, switchToDeviceTab)
    , mPlater(plater)
    , mPrintPlateIdx(printPlateIdx)
    , mTimeLapse(0)
    , mHeatedBedLeveling(0)
    , mBedType(BedType::btPTE)
    , mFilamentAmsList(filamentAmsList)
{}

PrintSendDialogEx::~PrintSendDialogEx() {}

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

    mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrintSendDialogEx::onScriptMessage, this);

    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/print_send/printsend.html").make_preferred().string());
    TargetUrl          = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    topsizer->Add(mBrowser, wxSizerFlags().Expand().Proportion(1));
    int height = 815;
    if (mFilamentAmsList.empty()) {
        height = 564;
    }
    wxSize pSize = FromDIP(wxSize(860, height));
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

void PrintSendDialogEx::onScriptMessage(wxWebViewEvent& evt)
{
    try {
        wxString strInput = evt.GetString();
        BOOST_LOG_TRIVIAL(trace) << "PrintSendDialogEx::OnScriptMessage;OnRecv:" << strInput.c_str();
        json     j      = json::parse(strInput);
        wxString strCmd = j["command"];
        BOOST_LOG_TRIVIAL(trace) << "PrintSendDialogEx::OnScriptMessage;Command:" << strCmd;

        if (strCmd == "request_print_task") {
            json response           = json::object();
            response["command"]     = "response_print_task";
            response["sequence_id"] = "10001";
            response["response"]    = preparePrintTask();

            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));

            BOOST_LOG_TRIVIAL(trace) << "PrintSendDialogEx::OnScriptMessage;response_ams_filament_list:" << strJS.c_str();
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (strCmd == "request_printer_list") {
            json response           = json::object();
            response["command"]     = "response_printer_list";
            response["sequence_id"] = "10002";
            response["response"]    = getPrinterList();

            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));

            BOOST_LOG_TRIVIAL(trace) << "PrintSendDialogEx::OnScriptMessage;response_printer_list:" << strJS.c_str();
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (strCmd == "cancel_print") {
            onCancel();
        } else if (strCmd == "start_upload") {
            json printInfo = j["data"]["printInfo"];
            onPrint(printInfo);
        }

    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(trace) << "PrintSendDialogEx::OnScriptMessage;Error:" << e.what();
    }
}

void PrintSendDialogEx::runScript(const wxString& javascript)
{
    if (!mBrowser)
        return;

    WebView::RunScript(mBrowser, javascript);
}

nlohmann::json PrintSendDialogEx::preparePrintTask()
{
    nlohmann::json           printTask        = json::object();
    auto                     preset_bundle    = wxGetApp().preset_bundle;
    nlohmann::json           amsInfo          = json::object();
    nlohmann::json           trayFilamentList = json::array();
    std::vector<std::string> trayIndexList    = {"A1", "A2", "A3", "A4", "B1", "B2", "B3", "B4",
                                                 "C1", "C2", "C3", "C4", "D1", "D2", "D3", "D4"};
    for (int i = 0; i < mFilamentAmsList.size(); i++) {
        auto&          ams           = mFilamentAmsList[i];
        nlohmann::json filament      = json::object();
        filament["filamentId"]       = ams.opt_string("filament_id", 0u);
        filament["trayId"]           = ams.opt_string("tray_id", 0u);
        filament["amsFilamentColor"] = ams.opt_string("filament_colour", 0u);
        filament["amsFilamentType"]  = ams.opt_string("filament_type", 0u);
        filament["amsFilamentName"]  = ams.opt_string("filament_name", 0u);
        filament["amsId"]            = ams.opt_string("ams_id", 0u);
        filament["trayIndex"]        = trayIndexList[i];
        trayFilamentList.push_back(filament);
    }
    amsInfo["trayFilamentList"] = trayFilamentList;
    printTask["amsInfo"]        = amsInfo;

    nlohmann::json printInfo    = json::object();
    nlohmann::json filamentList = json::array();

    const Print& print       = mPlater->get_partplate_list().get_current_fff_print();
    const auto&  stats       = print.print_statistics();
    printInfo["printTime"]   = stats.estimated_normal_print_time;
    printInfo["totalWeight"] = stats.total_weight;

    int layerCount = 0;
    for (const PrintObject* object : print.objects()) {
        layerCount = std::max(layerCount, (int) object->layer_count());
    }
    printInfo["layerCount"] = layerCount;

    std::string modelName = txt_filename->GetValue().ToUTF8().data();
    if (modelName.size() >= 6 && modelName.compare(modelName.size() - 6, 6, ".gcode") == 0)
        modelName = modelName.substr(0, modelName.size() - 6);

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

    struct FilamentInfo
    {
        std::string filamentType;
        std::string filamentId;
        std::string filamentName;
        std::string weight;
        float       density;
    };
    std::vector<FilamentInfo> allFilaments;

    for (const auto& filamentName : preset_bundle->filament_presets) {
        auto it = nameToPreset.find(filamentName);
        if (it != nameToPreset.end()) {
            Preset*     preset = it->second;
            std::string displayFilamentType;
            std::string filamentType = preset->config.get_filament_type(displayFilamentType);

            float                     density            = 0.0;
            const ConfigOptionFloats* filament_densities = preset->config.option<ConfigOptionFloats>("filament_density");
            if (filament_densities != nullptr) {
                density = filament_densities->values[0];
            }
            std::string displayFilamentName = filamentName;
            size_t pos = displayFilamentName.find('@');
            if (pos != std::string::npos) {
                displayFilamentName = displayFilamentName.substr(0, pos);
            }
            allFilaments.push_back(FilamentInfo{
                filamentType,
                preset->filament_id,
                displayFilamentName,
                "0",
                density,
            });
        }
    }

    auto extruders = wxGetApp().plater()->get_partplate_list().get_curr_plate()->get_used_extruders();
    for (auto i = 0; i < extruders.size(); i++) {
        int extruderIdx = extruders[i] - 1;
        if (extruderIdx < 0 || extruderIdx >= (int) allFilaments.size())
            continue;
        auto& info = allFilaments[extruderIdx];

        auto  colour       = wxGetApp().preset_bundle->project_config.opt_string("filament_colour", (unsigned int) extruderIdx);
        float total_weight = 0.0;

        const auto& stats = print.print_statistics();
        

        double model_volume_mm3 = 0.0;
        auto model_it = stats.filament_stats.find(extruderIdx);
        if (model_it != stats.filament_stats.end()) {
            model_volume_mm3 = model_it->second;
        }
        
        double wipe_tower_volume_mm3 = 0.0;
        double support_volume_mm3 = 0.0;

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
            double raw_weight = total_volume_mm3 * info.density * 0.001;

            if (raw_weight > 0) {
                double magnitude = pow(10, floor(log10(raw_weight)));
                double normalized = raw_weight / magnitude;
                total_weight = round(normalized * 100) / 100 * magnitude;
            } else {
                total_weight = 0.0;
            }
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << total_weight;
        info.weight = ss.str() + "g";

        nlohmann::json filament = {{"filamentId", info.filamentId},
                                   {"filamentColor", colour},
                                   {"filamentType", info.filamentType},
                                   {"filamentName", info.filamentName},
                                   {"filamentWeight", info.weight},
                                   {"trayIndex", ""},
                                   {"trayId", ""},
                                   {"amsId", ""},
                                   {"amsFilamentName", ""},
                                   {"amsFilamentColor", ""},
                                   {"amsFilamentType", ""}};
        filamentList.push_back(filament);
    }
    getFilamentAmsMapping(filamentList, trayFilamentList);
    printInfo["filamentList"] = filamentList;
    printTask["printInfo"]    = printInfo;

    return printTask;
}

nlohmann::json PrintSendDialogEx::getPrinterList()
{
    nlohmann::json printers = json::array();
    std::vector<PrinterNetworkInfo> printerList = PrinterManager::getInstance()->getPrinterList();
    for (auto& printer : printerList) {
        nlohmann::json printerJson = json::object();
        printerJson = PrinterManager::convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
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

void PrintSendDialogEx::getFilamentAmsMapping(nlohmann::json& filamentList, nlohmann::json& trayFilamentList)
{
    AppConfig* app_config = wxGetApp().app_config;
    for (auto& filament : filamentList) {
        std::string filamentStandardColor = getStandardColor(filament["filamentColor"].get<std::string>());
        std::string key                   = filament["filamentId"].get<std::string>() + "_" + filamentStandardColor;
        std::string amsFilamentInfo       = app_config->get("ams_filament_mapping", key);
        std::string amsFilamentName       = "";
        std::string amsFilamentColor      = "";
        bool        isMapped              = false;
        // already mapped
        if (!amsFilamentInfo.empty()) {
            std::vector<std::string> amsFilamentInfoList;
            boost::split(amsFilamentInfoList, amsFilamentInfo, boost::is_any_of("_"));
            if (amsFilamentInfoList.size() == 2) {
                amsFilamentName  = amsFilamentInfoList[0];
                amsFilamentColor = amsFilamentInfoList[1];
            }
        }
        if (!amsFilamentName.empty() && !amsFilamentColor.empty()) {
            for (auto& trayFilament : trayFilamentList) {
                if (trayFilament["amsFilamentName"].get<std::string>() == amsFilamentName &&
                    trayFilament["amsFilamentColor"].get<std::string>() == amsFilamentColor) {
                    filament["trayIndex"]        = trayFilament["trayIndex"].get<std::string>();
                    filament["trayId"]           = trayFilament["trayId"].get<std::string>();
                    filament["amsId"]            = trayFilament["amsId"].get<std::string>();
                    filament["amsFilamentType"]  = trayFilament["amsFilamentType"].get<std::string>();
                    filament["amsFilamentName"]  = trayFilament["amsFilamentName"].get<std::string>();
                    filament["amsFilamentColor"] = trayFilament["amsFilamentColor"].get<std::string>();
                    isMapped                     = true;
                    break;
                }
            }
        }

        if (isMapped) {
            continue;
        }
        // not mapped or mapped filament not exist
        for (auto& trayFilament : trayFilamentList) {
            if (trayFilament["filamentId"].get<std::string>() == filament["filamentId"].get<std::string>() &&
                trayFilament["amsFilamentColor"].get<std::string>() == filamentStandardColor) {
                filament["trayIndex"]        = trayFilament["trayIndex"].get<std::string>();
                filament["trayId"]           = trayFilament["trayId"].get<std::string>();
                filament["amsId"]            = trayFilament["amsId"].get<std::string>();
                filament["amsFilamentName"]  = trayFilament["amsFilamentName"].get<std::string>();
                filament["amsFilamentColor"] = trayFilament["amsFilamentColor"].get<std::string>();
                filament["amsFilamentType"]  = trayFilament["amsFilamentType"].get<std::string>();
                break;
            }
        }
    }
}

void PrintSendDialogEx::saveFilamentAmsMapping(nlohmann::json& filamentList)
{
    AppConfig* app_config = wxGetApp().app_config;
    app_config->clear_section("ams_filament_mapping");
    for (auto& filament : filamentList) {
        if (filament["trayIndex"].get<std::string>().empty() || filament["trayId"].get<std::string>().empty() ||
            filament["amsId"].get<std::string>().empty() || filament["amsFilamentName"].get<std::string>().empty() ||
            filament["amsFilamentColor"].get<std::string>().empty()) {
            continue;
        }
        std::string filamentStandardColor = getStandardColor(filament["filamentColor"].get<std::string>());
        std::string key                   = filament["filamentId"].get<std::string>() + "_" + filamentStandardColor;
        std::string amsInfo = filament["amsFilamentName"].get<std::string>() + "_" + filament["amsFilamentColor"].get<std::string>();
        app_config->set("ams_filament_mapping", key, amsInfo);
    }
}
void PrintSendDialogEx::onPrint(const nlohmann::json& printInfo)
{
    try {
        mSelectedPrinterId = "";
        mTimeLapse         = printInfo["timeLapse"].get<bool>();
        mHeatedBedLeveling = printInfo["heatedBedLeveling"].get<bool>();
        mAutoRefill        = printInfo["autoRefill"].get<bool>();
        bool uploadAndPrint    = printInfo["uploadAndPrint"].get<bool>();
        m_switch_to_device_tab = printInfo["switchToDeviceTab"].get<bool>();
        mSelectedPrinterId = printInfo["selectedPrinterId"].get<std::string>();
        std::string bedType = printInfo["bedType"].get<std::string>();

        if(bedType == "btPC") {
            mBedType = BedType::btPC;
        } else {
            mBedType = BedType::btPEI;
        }

        if(!mSelectedPrinterId.empty()) {
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
        if (uploadAndPrint && mFilamentAmsList.size() > 0) {
            nlohmann::json        filamentList = json::array();
            bool                  hasMapped    = false;
            std::set<std::string> amsIdSet;
            for (size_t i = 0; i < printInfo["filamentList"].size(); i++) {
                nlohmann::json filament = printInfo["filamentList"][i];
                filamentList.push_back(filament);
                std::string color        = filament.value("filamentColor", "");
                std::string amsId        = filament.value("amsId", "");
                std::string trayId       = filament.value("trayId", "");
                std::string filamentType = filament.value("filamentType", "");
                hasMapped                = true;
                amsIdSet.insert(amsId);
                if (amsId.empty() || color.empty() || trayId.empty() || filamentType.empty()) {
                    hasMapped = false;
                    break;
                }
            }
            if (hasMapped) {
                mFilamentAmsMapping                 = nlohmann::json::object();
                mFilamentAmsMapping["amsNum"]       = amsIdSet.size();
                mFilamentAmsMapping["colorNum"]     = filamentList.size();
                mFilamentAmsMapping["filamentList"] = filamentList;
                saveFilamentAmsMapping(filamentList);
            } else {
                wxMessageBox("Some filaments are not mapped!", "Error", wxOK | wxICON_ERROR);
                EndModal(wxID_CANCEL);
                return;
            }
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

}} // namespace Slic3r::GUI
