#include "ElegooPrintSend.hpp"
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

static const char* CONFIG_KEY_PATH    = "printhost_path";
static const char* CONFIG_KEY_GROUP   = "printhost_group";
static const char* CONFIG_KEY_STORAGE = "printhost_storage";

static const char* CONFIG_KEY_UPLOADANDPRINT    = "elegoolink_upload_and_print";
static const char* CONFIG_KEY_TIMELAPSE         = "elegoolink_timelapse";
static const char* CONFIG_KEY_HEATEDBEDLEVELING = "elegoolink_heated_bed_leveling";
static const char* CONFIG_KEY_BEDTYPE           = "elegoolink_bed_type";
static const char* CONFIG_KEY_AUTO_REFILL       = "elegoolink_auto_refill";

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

ElegooPrintSend::ElegooPrintSend(Plater*                                  plater,
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

ElegooPrintSend::~ElegooPrintSend() {}

void ElegooPrintSend::init()
{
    const AppConfig* app_config = wxGetApp().app_config;

    auto preset_bundle = wxGetApp().preset_bundle;
    auto model_id      = preset_bundle->printers.get_edited_preset().get_printer_type(preset_bundle);

    if (!(model_id == "Elegoo-CC" || model_id == "Elegoo-C")) {
        PrintHostSendDialog::init();
        return;
    }

    SetIcon(wxNullIcon);
    // DestroyChildren();
    mBrowser = WebView::CreateWebView(this, "");
    if (mBrowser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    mBrowser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &ElegooPrintSend::onScriptMessage, this);

    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/device/elegoo_print_send/printsend.html").make_preferred().string());
    TargetUrl          = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    topsizer->Add(mBrowser, wxSizerFlags().Expand().Proportion(1));
    int height = 740;
    if (mFilamentAmsList.empty()) {
        height = 500;
    }
    wxSize pSize = FromDIP(wxSize(446, height));
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

std::string ElegooPrintSend::getCurrentProjectName()
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

void ElegooPrintSend::onScriptMessage(wxWebViewEvent& evt)
{
    try {
        wxString strInput = evt.GetString();
        BOOST_LOG_TRIVIAL(trace) << "ElegooPrintSend::OnScriptMessage;OnRecv:" << strInput.c_str();
        json     j      = json::parse(strInput);
        wxString strCmd = j["command"];
        BOOST_LOG_TRIVIAL(trace) << "ElegooPrintSend::OnScriptMessage;Command:" << strCmd;

        if (strCmd == "request_print_task") {
            json response           = json::object();
            response["command"]     = "response_print_task";
            response["sequence_id"] = "10001";
            response["response"]    = preparePrintTask();

            wxString strJS = wxString::Format("HandleStudio(%s)", response.dump(-1, ' ', true));

            BOOST_LOG_TRIVIAL(trace) << "ElegooPrintSend::OnScriptMessage;response_ams_filament_list:" << strJS.c_str();
            wxGetApp().CallAfter([this, strJS] { runScript(strJS); });
        } else if (strCmd == "cancel_print") {
            onCancel();
        } else if (strCmd == "start_upload") {
            json printInfo = j["data"]["printInfo"];
            onPrint(printInfo);
        }

    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(trace) << "ElegooPrintSend::OnScriptMessage;Error:" << e.what();
    }
}

void ElegooPrintSend::runScript(const wxString& javascript)
{
    if (!mBrowser)
        return;

    WebView::RunScript(mBrowser, javascript);
}

nlohmann::json ElegooPrintSend::preparePrintTask()
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

void ElegooPrintSend::getFilamentAmsMapping(nlohmann::json& filamentList, nlohmann::json& trayFilamentList)
{
    AppConfig* app_config = wxGetApp().app_config;
    for (auto& filament : filamentList) {
        std::string filamentStandardColor = getStandardColor(filament["filamentColor"].get<std::string>());
        std::string key                   = filament["filamentId"].get<std::string>() + "_" + filamentStandardColor;
        std::string amsFilamentInfo       = app_config->get("elegoo_ams_filament_mapping", key);
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

void ElegooPrintSend::saveFilamentAmsMapping(nlohmann::json& filamentList)
{
    AppConfig* app_config = wxGetApp().app_config;
    app_config->clear_section("elegoo_ams_filament_mapping");
    for (auto& filament : filamentList) {
        if (filament["trayIndex"].get<std::string>().empty() || filament["trayId"].get<std::string>().empty() ||
            filament["amsId"].get<std::string>().empty() || filament["amsFilamentName"].get<std::string>().empty() ||
            filament["amsFilamentColor"].get<std::string>().empty()) {
            continue;
        }
        std::string filamentStandardColor = getStandardColor(filament["filamentColor"].get<std::string>());
        std::string key                   = filament["filamentId"].get<std::string>() + "_" + filamentStandardColor;
        std::string amsInfo = filament["amsFilamentName"].get<std::string>() + "_" + filament["amsFilamentColor"].get<std::string>();
        app_config->set("elegoo_ams_filament_mapping", key, amsInfo);
    }
}

// void ElegooPrintSend::updata_thumbnail_data_after_connected_printer()
// {
//     // change thumbnail_data
//     ThumbnailData &input_data          = m_plater->get_partplate_list().get_curr_plate()->thumbnail_data;
//     ThumbnailData &no_light_data = m_plater->get_partplate_list().get_curr_plate()->no_light_thumbnail_data;
//     if (input_data.width == 0 || input_data.height == 0 || no_light_data.width == 0 || no_light_data.height == 0) {
//         wxGetApp().plater()->update_all_plate_thumbnails(false);
//     }
//     unify_deal_thumbnail_data(input_data, no_light_data);
// }

void ElegooPrintSend::unifyDealThumbnailData(ThumbnailData& inputData, ThumbnailData& noLightData)
{
    // if (input_data.width == 0 || input_data.height == 0 || no_light_data.width == 0 || no_light_data.height == 0) {
    //     BOOST_LOG_TRIVIAL(error) << "SelectMachineDialog::no_light_data is empty,error";
    //     return;
    // }
    // m_cur_input_thumbnail_data    = input_data;
    // m_cur_no_light_thumbnail_data = no_light_data;
    // clone_thumbnail_data();
    // MaterialHash::iterator iter               = m_materialList.begin();
    // bool                   is_connect_printer = true;
    // while (iter != m_materialList.end()) {
    //     int           id   = iter->first;
    //     Material *    item = iter->second;
    //     MaterialItem *m    = item->item;
    //     if (m->m_ams_name == "-") {
    //         is_connect_printer = false;
    //         break;
    //     }
    //     iter++;
    // }
    // if (is_connect_printer) {
    //     change_default_normal(-1, wxColour());
    //     final_deal_edge_pixels_data(m_preview_thumbnail_data);
    //     set_default_normal(m_preview_thumbnail_data);
    // }
}

void ElegooPrintSend::changeDefaultNormal(int oldFilamentId, wxColour tempAmsColor)
{
    if (mCurColorsInThumbnail.size() == 0) {
        BOOST_LOG_TRIVIAL(error) << "SelectMachineDialog::change_default_normal:error:m_cur_colors_in_thumbnail.size() == 0";
        return;
    }
    if (oldFilamentId >= 0) {
        if (oldFilamentId < mCurColorsInThumbnail.size()) {
            mCurColorsInThumbnail[oldFilamentId] = tempAmsColor;
        } else {
            BOOST_LOG_TRIVIAL(error)
                << "SelectMachineDialog::change_default_normal:error:old_filament_id > m_cur_colors_in_thumbnail.size()";
            return;
        }
    }
    ThumbnailData& data        = mCurInputThumbnailData;
    ThumbnailData& noLightData = mCurNoLightThumbnailData;
    if (data.width > 0 && data.height > 0 && data.width == noLightData.width && data.height == noLightData.height) {
        for (unsigned int r = 0; r < data.height; ++r) {
            unsigned int rr = (data.height - 1 - r) * data.width;
            for (unsigned int c = 0; c < data.width; ++c) {
                unsigned char* noLightPx = (unsigned char*) noLightData.pixels.data() + 4 * (rr + c);
                unsigned char* originPx  = (unsigned char*) data.pixels.data() + 4 * (rr + c);
                unsigned char* newPx     = (unsigned char*) mPreviewThumbnailData.pixels.data() + 4 * (rr + c);
                if (originPx[3] > 0 && mEdgePixels[r * data.width + c] == false) {
                    auto filamentId = 255 - noLightPx[3];
                    if (filamentId >= mCurColorsInThumbnail.size()) {
                        continue;
                    }
                    wxColour tempAmsColorInLoop = mCurColorsInThumbnail[filamentId];
                    wxColour amsColor           = adjustColorForRender(tempAmsColorInLoop);
                    // change color
                    newPx[3]                   = originPx[3]; // alpha
                    int           originRgb    = originPx[0] + originPx[1] + originPx[2];
                    int           noLightPxRgb = noLightPx[0] + noLightPx[1] + noLightPx[2];
                    unsigned char i            = 0;
                    if (originRgb >= noLightPxRgb) { // Brighten up
                        unsigned char curSingleColor = amsColor.Red();
                        newPx[i]                     = std::clamp(curSingleColor + (originPx[i] - noLightPx[i]), 0, 255);
                        i++;
                        curSingleColor = amsColor.Green();
                        newPx[i]       = std::clamp(curSingleColor + (originPx[i] - noLightPx[i]), 0, 255);
                        i++;
                        curSingleColor = amsColor.Blue();
                        newPx[i]       = std::clamp(curSingleColor + (originPx[i] - noLightPx[i]), 0, 255);
                    } else { // Dimming
                        float         ratio          = originRgb / (float) noLightPxRgb;
                        unsigned char curSingleColor = amsColor.Red();
                        newPx[i]                     = std::clamp((int) (curSingleColor * ratio), 0, 255);
                        i++;
                        curSingleColor = amsColor.Green();
                        newPx[i]       = std::clamp((int) (curSingleColor * ratio), 0, 255);
                        i++;
                        curSingleColor = amsColor.Blue();
                        newPx[i]       = std::clamp((int) (curSingleColor * ratio), 0, 255);
                    }
                }
            }
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "SelectMachineDialog::change_defa:no_light_data is empty,error";
    }
}

void ElegooPrintSend::cloneThumbnailData()
{
    // record preview_colors
    //  MaterialHash::iterator iter               = m_materialList.begin();
    //  if (m_preview_colors_in_thumbnail.size() != m_materialList.size()) {
    //      m_preview_colors_in_thumbnail.resize(m_materialList.size());
    //  }
    //  while (iter != m_materialList.end()) {
    //      int           id   = iter->first;
    //      Material *    item = iter->second;
    //      MaterialItem *m    = item->item;
    //      m_preview_colors_in_thumbnail[id] = m->m_material_coloul;
    //      if (item->id < m_cur_colors_in_thumbnail.size()) {
    //          m_cur_colors_in_thumbnail[item->id] = m->m_ams_coloul;
    //      }
    //      else {//exist empty or unrecognized type ams in machine
    //          m_cur_colors_in_thumbnail.resize(item->id + 1);
    //          m_cur_colors_in_thumbnail[item->id] = m->m_ams_coloul;
    //      }
    //      iter++;
    //  }
    //  //copy data
    //  auto &data   = m_cur_input_thumbnail_data;
    //  m_preview_thumbnail_data.reset();
    //  m_preview_thumbnail_data.set(data.width, data.height);
    //  if (data.width > 0 && data.height > 0) {
    //      for (unsigned int r = 0; r < data.height; ++r) {
    //          unsigned int rr = (data.height - 1 - r) * data.width;
    //          for (unsigned int c = 0; c < data.width; ++c) {
    //              unsigned char *origin_px   = (unsigned char *) data.pixels.data() + 4 * (rr + c);
    //              unsigned char *new_px      = (unsigned char *) m_preview_thumbnail_data.pixels.data() + 4 * (rr + c);
    //              for (size_t i = 0; i < 4; i++) {
    //                  new_px[i] = origin_px[i];
    //              }
    //          }
    //      }
    //  }
    //  //record_edge_pixels_data
    //  record_edge_pixels_data();
}

void ElegooPrintSend::recordEdgePixelsData()
{
    // auto is_not_in_preview_colors = [this](unsigned char r, unsigned char g , unsigned char b , unsigned char a) {
    //     for (size_t i = 0; i < m_preview_colors_in_thumbnail.size(); i++) {
    //         wxColour  render_color  = adjust_color_for_render(m_preview_colors_in_thumbnail[i]);
    //         if (render_color.Red() == r && render_color.Green() == g && render_color.Blue() == b /*&& render_color.Alpha() == a*/) {
    //             return false;
    //         }
    //     }
    //     return true;
    // };
    // ThumbnailData &data = m_cur_no_light_thumbnail_data;
    // ThumbnailData &origin_data = m_cur_input_thumbnail_data;
    // if (data.width > 0 && data.height > 0) {
    //     m_edge_pixels.resize(data.width * data.height);
    //     for (unsigned int r = 0; r < data.height; ++r) {
    //         unsigned int rr        = (data.height - 1 - r) * data.width;
    //         for (unsigned int c = 0; c < data.width; ++c) {
    //             unsigned char *no_light_px = (unsigned char *) data.pixels.data() + 4 * (rr + c);
    //             unsigned char *origin_px          = (unsigned char *) origin_data.pixels.data() + 4 * (rr + c);
    //             m_edge_pixels[r * data.width + c] = false;
    //             if (origin_px[3] > 0) {
    //                 if (is_not_in_preview_colors(no_light_px[0], no_light_px[1], no_light_px[2], origin_px[3])) {
    //                     m_edge_pixels[r * data.width + c] = true;
    //                 }
    //             }
    //         }
    //     }
    // }
}
wxColour ElegooPrintSend::adjustColorForRender(const wxColour& color)
{
    ColorRGBA _tempColorColor(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);
    auto      _tempColorColor_ = adjust_color_for_rendering(_tempColorColor);
    wxColour  renderColor((int) (_tempColorColor_[0] * 255.0f), (int) (_tempColorColor_[1] * 255.0f), (int) (_tempColorColor_[2] * 255.0f),
                          (int) (_tempColorColor_[3] * 255.0f));
    return renderColor;
}

void ElegooPrintSend::onPrint(const nlohmann::json& printInfo)
{
    try {
        bool timeLapse         = printInfo["timeLapse"].get<bool>();
        bool heatedBedLeveling = printInfo["heatedBedLeveling"].get<bool>();
        bool uploadAndPrint    = printInfo["uploadAndPrint"].get<bool>();
        bool switchToDeviceTab = printInfo["switchToDeviceTab"].get<bool>();
        bool auto_refill       = printInfo["autoRefill"].get<bool>();

        mTimeLapse             = timeLapse ? 1 : 0;
        mHeatedBedLeveling     = heatedBedLeveling ? 1 : 0;
        post_upload_action     = uploadAndPrint ? PrintHostPostUploadAction::StartPrint : PrintHostPostUploadAction::None;
        m_switch_to_device_tab = switchToDeviceTab ? 1 : 0;
        mAutoRefill            = auto_refill ? 1 : 0;

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

void ElegooPrintSend::onCancel() { EndModal(wxID_CANCEL); }

void ElegooPrintSend::finalDealEdgePixelsData(ThumbnailData& data)
{
    // if (data.width > 0 && data.height > 0 && m_edge_pixels.size() >0 ) {
    //     for (unsigned int r = 0; r < data.height; ++r) {
    //          unsigned int rr            = (data.height - 1 - r) * data.width;
    //          bool         exist_rr_up   = r >= 1 ? true : false;
    //          bool         exist_rr_down = r <= data.height - 2 ? true : false;
    //          unsigned int rr_up         = exist_rr_up ? (data.height - 1 - (r - 1)) * data.width : 0;
    //          unsigned int rr_down       = exist_rr_down ? (data.height - 1 - (r + 1)) * data.width : 0;
    //          for (unsigned int c = 0; c < data.width; ++c) {
    //               bool         exist_c_left  = c >= 1 ? true : false;
    //               bool         exist_c_right = c <= data.width - 2 ? true : false;
    //               unsigned int c_left        = exist_c_left ? c - 1 : 0;
    //               unsigned int c_right       = exist_c_right ? c + 1 : 0;
    //               unsigned char *cur_px   = (unsigned char *) data.pixels.data() + 4 * (rr + c);
    //               unsigned char *relational_pxs[8] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    //               if (exist_rr_up && exist_c_left) { relational_pxs[0] = (unsigned char *) data.pixels.data() + 4 * (rr_up + c_left); }
    //               if (exist_rr_up) { relational_pxs[1] = (unsigned char *) data.pixels.data() + 4 * (rr_up + c); }
    //               if (exist_rr_up && exist_c_right) { relational_pxs[2] = (unsigned char *) data.pixels.data() + 4 * (rr_up + c_right); }
    //               if (exist_c_left) { relational_pxs[3] = (unsigned char *) data.pixels.data() + 4 * (rr + c_left); }
    //               if (exist_c_right) { relational_pxs[4] = (unsigned char *) data.pixels.data() + 4 * (rr + c_right); }
    //               if (exist_rr_down && exist_c_left) { relational_pxs[5] = (unsigned char *) data.pixels.data() + 4 * (rr_down + c_left);
    //               } if (exist_rr_down) { relational_pxs[6] = (unsigned char *) data.pixels.data() + 4 * (rr_down + c); } if
    //               (exist_rr_down && exist_c_right) { relational_pxs[7] = (unsigned char *) data.pixels.data() + 4 * (rr_down + c_right);
    //               } if (cur_px[3] > 0 && m_edge_pixels[r * data.width + c]) {
    //                    int rgba_sum[4] = {0, 0, 0, 0};
    //                    int valid_count = 0;
    //                    for (size_t k = 0; k < 8; k++) {
    //                        if (relational_pxs[k]) {
    //                            if (k == 0 && m_edge_pixels[(r - 1) * data.width + c_left]) {
    //                                 continue;
    //                             }
    //                            if (k == 1 && m_edge_pixels[(r - 1) * data.width + c]) {
    //                                 continue;
    //                             }
    //                             if (k == 2 && m_edge_pixels[(r - 1) * data.width + c_right]) {
    //                                 continue;
    //                             }
    //                             if (k == 3 && m_edge_pixels[r * data.width + c_left]) {
    //                                 continue;
    //                             }
    //                             if (k == 4 && m_edge_pixels[r * data.width + c_right]) {
    //                                 continue;
    //                             }
    //                             if (k == 5 && m_edge_pixels[(r + 1) * data.width + c_left]) {
    //                                 continue;
    //                             }
    //                             if (k == 6 && m_edge_pixels[(r + 1) * data.width + c]) {
    //                                 continue;
    //                             }
    //                             if (k == 7 && m_edge_pixels[(r + 1) * data.width + c_right]) {
    //                                 continue;
    //                             }
    //                             for (size_t m = 0; m < 4; m++) {
    //                                 rgba_sum[m] += relational_pxs[k][m];
    //                             }
    //                             valid_count++;
    //                        }
    //                    }
    //                    if (valid_count > 0) {
    //                         for (size_t m = 0; m < 4; m++) {
    //                             cur_px[m] = std::clamp(int(rgba_sum[m] / (float)valid_count), 0, 255);
    //                         }
    //                    }
    //               }
    //          }
    //     }
    // }
}

void ElegooPrintSend::EndModal(int ret)
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

BedType ElegooPrintSend::appBedType() const
{
    std::string str_bed_type   = wxGetApp().app_config->get("curr_bed_type");
    int         bed_type_value = atoi(str_bed_type.c_str());
    return static_cast<BedType>(bed_type_value);
}

void ElegooPrintSend::refresh()
{
    // if (uploadandprint_sizer) {
    //     if (post_upload_action == PrintHostPostUploadAction::StartPrint) {
    //         uploadandprint_sizer->Show(true);
    //     } else {
    //         uploadandprint_sizer->Show(false);
    //     }
    // }
    // if (warning_text) {
    //     warning_text->Show(post_upload_action == PrintHostPostUploadAction::StartPrint && appBedType() != m_BedType);
    // }
    // this->Layout();
    // this->Fit();
}
}} // namespace Slic3r::GUI
