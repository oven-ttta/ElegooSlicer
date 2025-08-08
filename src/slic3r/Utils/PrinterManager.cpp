#include "PrinterManager.hpp"
#include "slic3r/Utils/PrinterNetworkManager.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "libslic3r/PresetBundle.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "slic3r/Utils/PrintHost.hpp"
#include <boost/beast/core/detail/base64.hpp>
namespace Slic3r {

VendorProfile getMachineProfile(const std::string& vendorName, const std::string& machineModel, VendorProfile::PrinterModel& printerModel)
{
    std::string   profile_vendor_name;
    VendorProfile machineProfile;
    PresetBundle  bundle;
    auto [substitutions, errors] = bundle.load_system_models_from_json(ForwardCompatibilitySubstitutionRule::EnableSilent);
    for (const auto& vendor : bundle.vendors) {
        const auto& vendor_profile = vendor.second;
        if (boost::to_upper_copy(vendor_profile.name) == boost::to_upper_copy(vendorName)) {
            // find the profile model name from the vendor profile
            // The profile model name may not contain the vendor name, so we need to add the vendor name
            for (const auto& model : vendor_profile.models) {
                std::string profileModelName     = boost::to_upper_copy(model.name);
                std::string discoverMachineModel = boost::to_upper_copy(machineModel);

                if (profileModelName.find(boost::to_upper_copy(vendorName)) == std::string::npos) {
                    profileModelName = boost::to_upper_copy(vendorName) + " " + profileModelName;
                }

                if (discoverMachineModel.find(boost::to_upper_copy(vendorName)) == std::string::npos) {
                    discoverMachineModel = boost::to_upper_copy(vendorName) + " " + discoverMachineModel;
                }

                if (profileModelName == discoverMachineModel) {
                    machineProfile = vendor_profile;
                    printerModel   = model;
                    break;
                }
            }
            break;
        }
    }
    return machineProfile;
}
std::string PrinterManager::imageFileToBase64DataURI(const std::string& image_path) {
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

PrinterNetworkInfo PrinterManager::convertJsonToPrinterNetworkInfo(const nlohmann::json& json)
{
    PrinterNetworkInfo printerNetworkInfo;
    try {
        printerNetworkInfo.printerId         = json["printerId"];
        printerNetworkInfo.printerName       = json["printerName"];
        printerNetworkInfo.host              = json["host"];
        printerNetworkInfo.port              = json["port"].get<int>();
        printerNetworkInfo.vendor            = json["vendor"];
        printerNetworkInfo.printerModel      = json["printerModel"];
        printerNetworkInfo.protocolVersion   = json["protocolVersion"];
        printerNetworkInfo.firmwareVersion   = json["firmwareVersion"];
        printerNetworkInfo.hostType          = json["hostType"];
        printerNetworkInfo.mainboardId       = json["mainboardId"];
        printerNetworkInfo.deviceType        = json["deviceType"];
        printerNetworkInfo.serialNumber      = json["serialNumber"];
        printerNetworkInfo.username          = json["username"];
        printerNetworkInfo.password          = json["password"];
        printerNetworkInfo.authMode          = json["authMode"];
        printerNetworkInfo.webUrl            = json["webUrl"];
        printerNetworkInfo.connectionUrl     = json["connectionUrl"];
        printerNetworkInfo.extraInfo         = json["extraInfo"];
        printerNetworkInfo.isPhysicalPrinter = json["isPhysicalPrinter"].get<bool>();
        printerNetworkInfo.addTime           = json["addTime"].get<uint64_t>();
        printerNetworkInfo.modifyTime        = json["modifyTime"].get<uint64_t>();
        printerNetworkInfo.lastActiveTime    = json["lastActiveTime"].get<uint64_t>();
        printerNetworkInfo.connectStatus     = static_cast<PrinterConnectStatus>(json["connectStatus"].get<int>());
        printerNetworkInfo.printerStatus     = static_cast<PrinterStatus>(json["printerStatus"].get<int>());
        if (json.contains("printTask")) {
            printerNetworkInfo.printTask.taskId        = json["printTask"]["taskId"];
            printerNetworkInfo.printTask.fileName      = json["printTask"]["fileName"];
            printerNetworkInfo.printTask.totalTime     = json["printTask"]["totalTime"].get<int>();
            printerNetworkInfo.printTask.currentTime   = json["printTask"]["currentTime"].get<int>();
            printerNetworkInfo.printTask.estimatedTime = json["printTask"]["estimatedTime"].get<int>();
            printerNetworkInfo.printTask.progress      = json["printTask"]["progress"].get<int>();
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to convert json to printer network info: " + std::string(e.what()));
    }
    return printerNetworkInfo;
}
nlohmann::json PrinterManager::convertPrinterNetworkInfoToJson(const PrinterNetworkInfo& printerNetworkInfo)
{
    nlohmann::json json;
    json["printerId"]         = printerNetworkInfo.printerId;
    json["printerName"]       = printerNetworkInfo.printerName;
    json["host"]              = printerNetworkInfo.host;
    json["port"]              = printerNetworkInfo.port;
    json["vendor"]            = printerNetworkInfo.vendor;
    json["printerModel"]      = printerNetworkInfo.printerModel;
    json["protocolVersion"]   = printerNetworkInfo.protocolVersion;
    json["firmwareVersion"]   = printerNetworkInfo.firmwareVersion;
    json["hostType"]          = printerNetworkInfo.hostType;
    json["mainboardId"]       = printerNetworkInfo.mainboardId;
    json["deviceType"]        = printerNetworkInfo.deviceType;
    json["serialNumber"]      = printerNetworkInfo.serialNumber;
    json["username"]          = printerNetworkInfo.username;
    json["password"]          = printerNetworkInfo.password;
    json["authMode"]          = printerNetworkInfo.authMode;
    json["webUrl"]            = printerNetworkInfo.webUrl;
    json["connectionUrl"]     = printerNetworkInfo.connectionUrl;
    json["extraInfo"]         = printerNetworkInfo.extraInfo;
    json["isPhysicalPrinter"] = printerNetworkInfo.isPhysicalPrinter;
    json["addTime"]           = printerNetworkInfo.addTime;
    json["modifyTime"]        = printerNetworkInfo.modifyTime;
    json["lastActiveTime"]    = printerNetworkInfo.lastActiveTime;
    json["connectStatus"]     = printerNetworkInfo.connectStatus;
    json["printerStatus"]     = printerNetworkInfo.printerStatus;
    nlohmann::json printTaskJson;
    printTaskJson["taskId"]        = printerNetworkInfo.printTask.taskId;
    printTaskJson["fileName"]      = printerNetworkInfo.printTask.fileName;
    printTaskJson["totalTime"]     = printerNetworkInfo.printTask.totalTime;
    printTaskJson["currentTime"]   = printerNetworkInfo.printTask.currentTime;
    printTaskJson["estimatedTime"] = printerNetworkInfo.printTask.estimatedTime;
    printTaskJson["progress"]      = printerNetworkInfo.printTask.progress;
    json["printTask"]              = printTaskJson;
    return json;
}

std::map<std::string, std::map<std::string, DynamicPrintConfig>> PrinterManager::getVendorPrinterModelConfig()
{
    std::map<std::string, std::map<std::string, DynamicPrintConfig>> vendorPrinterModelConfigMap;
    PresetBundle                                                     bundle;
    // load the system models from the resources dir
    auto [substitutions, errors] = bundle.load_system_models_from_json(ForwardCompatibilitySubstitutionRule::EnableSilent);

    for (const auto& vendor : bundle.vendors) {
        const std::string& vendorName = vendor.first;
        PresetBundle       vendorBundle;
        try {
            // load the vendor configs from the resources dir
            vendorBundle.load_vendor_configs_from_json((boost::filesystem::path(Slic3r::resources_dir()) / "profiles").string(), vendorName,
                                                       PresetBundle::LoadMachineOnly, ForwardCompatibilitySubstitutionRule::EnableSilent,
                                                       nullptr);
        } catch (const std::exception& e) {
            wxLogMessage("get printer model list load vendor %s error: %s", vendorName.c_str(), e.what());
            continue;
        }
        for (const auto& printer : vendorBundle.printers) {
            if (!printer.vendor) {
                continue;
            }
            std::string vendorName   = printer.vendor->name;
            auto        printerModel = printer.config.option<ConfigOptionString>("printer_model");
            if (!printerModel) {
                continue;
            }

            std::string modelName = printerModel->value;
            if (PrintHost::support_device_list_management(printer.config)) {
                vendorPrinterModelConfigMap[vendorName][modelName] = printer.config;
            }
        }
    }

    return vendorPrinterModelConfigMap;
}
PrinterManager::PrinterManager()
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

    PrinterNetworkManager::getInstance()
        ->registerCallBack([this](const std::string&          printerId,
                                  const PrinterConnectStatus& status) { updatePrinterConnectStatus(printerId, status); },
                           [this](const std::string& printerId, const PrinterStatus& status) { updatePrinterStatus(printerId, status); },
                           [this](const std::string& printerId, const PrinterPrintTask& task) { updatePrinterPrintTask(printerId, task); },
                           [this](const std::string& printerId, const PrinterNetworkInfo& printerInfo) { updatePrinterAttributes(printerId, printerInfo); });

    std::thread([this]() {
        for (const auto& [printerId, printer] : mPrinterList) {
            bool connected = false;
            auto result    = PrinterNetworkManager::getInstance()->addPrinter(printer, connected);
            if (result.isSuccess()) {
                updatePrinterConnectStatus(printerId, connected ? PRINTER_CONNECT_STATUS_CONNECTED : PRINTER_CONNECT_STATUS_DISCONNECTED);
            } else {
                updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
                wxLogWarning("Failed to add printer %s %s %s: %s", printer.host, printer.printerName, printer.printerModel,
                             result.message.c_str());
            }
        }
    }).detach();
}

PrinterManager::~PrinterManager() { close(); }

void PrinterManager::close() { PrinterNetworkManager::getInstance()->close(); }

bool PrinterManager::deletePrinter(const std::string& printerId)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        auto result = PrinterNetworkManager::getInstance()->disconnectFromPrinter(it->second);
        if (result.isError()) {
            wxLogWarning("Failed to disconnect printer %s %s %s during deletion: %s", it->second.host, it->second.printerName,
                         it->second.printerModel, result.message.c_str());
        }
        mPrinterList.erase(it);
        savePrinterList();
        return true;
    }
    return false;
}
bool PrinterManager::updatePrinterName(const std::string& printerId, const std::string& printerName)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        it->second.printerName = printerName;
        uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        it->second.modifyTime     = now;
        it->second.lastActiveTime = now;
        savePrinterList();
        return true;
    }
    return false;
}
bool PrinterManager::updatePrinterHost(const std::string& printerId, const std::string& host)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        it->second.host = host;
        uint64_t now    = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        it->second.modifyTime     = now;
        it->second.lastActiveTime = now;

        auto disconnectResult = PrinterNetworkManager::getInstance()->disconnectFromPrinter(it->second);
        if (disconnectResult.isError()) {
            wxLogWarning("Failed to disconnect printer %s %s %s during host update: %s", it->second.host, it->second.printerName,
                         it->second.printerModel, disconnectResult.message.c_str());
        }
        bool connected = false;
        auto addResult = PrinterNetworkManager::getInstance()->addPrinter(it->second, connected);
        savePrinterList();
        if(addResult.isSuccess()) {
            it->second.connectStatus = connected ? PRINTER_CONNECT_STATUS_CONNECTED : PRINTER_CONNECT_STATUS_DISCONNECTED;
            return true;
        } else {
            wxLogWarning("Failed to connect printer %s %s %s after host update: %s", it->second.host, it->second.printerName,
                         it->second.printerModel, addResult.message.c_str());
        }
    }
    return false;
}
std::string PrinterManager::addPrinter(PrinterNetworkInfo& printerNetworkInfo)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    // only generate a unique id for the printer when adding a printer
    // the printer info is from the UI, the UI info is from the discover device or manual add
    for (auto& p : mPrinterList) {
        if (p.second.host == printerNetworkInfo.host) {
            return p.second.printerId;
        }
    }
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printerNetworkInfo.addTime        = now;
    printerNetworkInfo.modifyTime     = now;
    printerNetworkInfo.lastActiveTime = now;
    printerNetworkInfo.printerId      = "";

    if(printerNetworkInfo.isPhysicalPrinter) {
        printerNetworkInfo.deviceType = PrinterNetworkManager::getDeviceType(printerNetworkInfo);
        if(printerNetworkInfo.deviceType == -1) {
            wxLogWarning("Failed to get device type for printer %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel);
            return "";
        }
    }
    printerNetworkInfo.printerId = boost::uuids::to_string(boost::uuids::random_generator{}());
    auto result = PrinterNetworkManager::getInstance()->connectToPrinter(printerNetworkInfo);
    if (result.isSuccess() && result.data.has_value()) {     
        auto connectedPrinter = result.data.value();
        printerNetworkInfo.connectStatus = PRINTER_CONNECT_STATUS_CONNECTED;
        if(printerNetworkInfo.isPhysicalPrinter) {
            printerNetworkInfo.mainboardId = connectedPrinter.mainboardId;
            printerNetworkInfo.serialNumber = connectedPrinter.serialNumber;
            printerNetworkInfo.deviceType = connectedPrinter.deviceType;
            printerNetworkInfo.webUrl        = connectedPrinter.webUrl;
            printerNetworkInfo.connectionUrl = connectedPrinter.connectionUrl;
            printerNetworkInfo.firmwareVersion = connectedPrinter.firmwareVersion;
            printerNetworkInfo.authMode = connectedPrinter.authMode;
            printerNetworkInfo.protocolVersion = connectedPrinter.protocolVersion;
        }

        mPrinterList[printerNetworkInfo.printerId] = printerNetworkInfo;
        savePrinterList();
        return printerNetworkInfo.printerId;
    } else {
        wxLogWarning("Failed to add printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                     printerNetworkInfo.printerModel, result.message.c_str());
    }

    return "";
}

std::vector<PrinterNetworkInfo> PrinterManager::discoverPrinter()
{
    std::lock_guard<std::mutex>     lock(mPrinterListMutex);
    boost::filesystem::path         resources_path(Slic3r::resources_dir());
    std::vector<PrinterNetworkInfo> printers;
    auto                            discoverResult = PrinterNetworkManager::getInstance()->discoverPrinters();

    if (!discoverResult.isSuccess()) {
        wxLogWarning("Failed to discover printers: %s", discoverResult.message.c_str());
        return printers;
    }

    std::vector<PrinterNetworkInfo> discoverPrinterList;
    if (discoverResult.hasData()) {
        discoverPrinterList = discoverResult.data.value();
    }
    if (discoverPrinterList.empty()) {
        return printers;
    }

    auto vendorPrinterModelConfigMap = getVendorPrinterModelConfig();
    for (auto& discoverPrinter : discoverPrinterList) {
        // check if the device is already bound, if it is, check if the ip, firmware version, etc. have changed and update them
        bool isSamePrinter = false;
        for (auto& p : mPrinterList) {
            if (!p.second.mainboardId.empty() && (discoverPrinter.mainboardId == p.second.mainboardId)) {
                isSamePrinter = true;
            }
            if (!p.second.serialNumber.empty() && (discoverPrinter.serialNumber == p.second.serialNumber)) {
                isSamePrinter = true;
            }
            if (isSamePrinter) {
                if (p.second.host != discoverPrinter.host) {
                    wxLogMessage("Printer %s %s %s IP changed, disconnect and connect to new IP, old IP: %s, new IP: %s", p.second.host,
                                 p.second.printerName, p.second.printerModel, p.second.host, discoverPrinter.host);
                    p.second.host            = discoverPrinter.host;
                    p.second.protocolVersion = discoverPrinter.protocolVersion;
                    p.second.firmwareVersion = discoverPrinter.firmwareVersion;
                    p.second.webUrl          = discoverPrinter.webUrl;
                    p.second.connectionUrl   = discoverPrinter.connectionUrl;
                    p.second.extraInfo       = discoverPrinter.extraInfo;
                    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                                       .count();
                    p.second.modifyTime     = now;
                    p.second.lastActiveTime = now;
                    savePrinterList();

                    auto disconnectResult = PrinterNetworkManager::getInstance()->disconnectFromPrinter(p.second);
                    if (disconnectResult.isError()) {
                        wxLogWarning("Failed to disconnect printer %s %s %s during IP change: %s", p.second.host, p.second.printerName,
                                     p.second.printerModel, disconnectResult.message.c_str());
                    }

                    auto connectResult = PrinterNetworkManager::getInstance()->connectToPrinter(p.second);
                    if (connectResult.isError()) {
                        wxLogWarning("Failed to connect printer %s %s %s after IP change: %s", p.second.host, p.second.printerName,
                                     p.second.printerModel, connectResult.message.c_str());
                    }
                }
                break;
            }
        }
        if (isSamePrinter) {
            continue;
        }
        // update the machine model and config info to keep consistent
        PrinterNetworkInfo printerNetworkInfo = discoverPrinter;
        printerNetworkInfo.printerId          = "";
        VendorProfile::PrinterModel printerModel;
        // get machine profile from resources dir and get host type from config
        VendorProfile machineProfile    = getMachineProfile(discoverPrinter.vendor, discoverPrinter.printerModel, printerModel);
        printerNetworkInfo.vendor       = machineProfile.name;
        printerNetworkInfo.printerName  = printerModel.name;
        printerNetworkInfo.printerModel = printerModel.name;
        if (printerNetworkInfo.deviceType == 2) {
            printerNetworkInfo.printerModel = "Elegoo Centauri Carbon 2";
        }
        printerNetworkInfo.isPhysicalPrinter = false;
        if (vendorPrinterModelConfigMap.find(printerNetworkInfo.vendor) != vendorPrinterModelConfigMap.end()) {
            auto modelConfigMap = vendorPrinterModelConfigMap[printerNetworkInfo.vendor];
            if (modelConfigMap.find(printerNetworkInfo.printerModel) != modelConfigMap.end()) {
                auto        config      = modelConfigMap[printerNetworkInfo.printerModel];
                const auto  opt         = config.option<ConfigOptionEnum<PrintHostType>>("host_type");
                const auto  hostType    = opt != nullptr ? opt->value : htOctoPrint;
                std::string hostTypeStr = PrintHost::get_print_host_type_str(hostType);
                if (!hostTypeStr.empty()) {
                    printerNetworkInfo.hostType = hostTypeStr;
                }
            }
        }
        printers.push_back(printerNetworkInfo);
    }
    return printers;
}
std::vector<PrinterNetworkInfo> PrinterManager::getPrinterList()
{
    std::lock_guard<std::mutex>     lock(mPrinterListMutex);
    std::vector<PrinterNetworkInfo> printers;
    for (auto& printerInfo : mPrinterList) {
        printers.push_back(printerInfo.second);
    }
    std::sort(printers.begin(), printers.end(),
              [](const PrinterNetworkInfo& a, const PrinterNetworkInfo& b) { return a.addTime < b.addTime; });
    return printers;
}
bool PrinterManager::upload(PrinterNetworkParams& params)
{
    std::string printerId = params.printerId;
    if (mPrinterList.find(printerId) == mPrinterList.end()) {
        if(params.errorFn) {
            params.errorFn("Printer not found, File name: " + params.fileName);
        }
        return false;
    }
    PrinterNetworkInfo printerNetworkInfo = mPrinterList[printerId];

    auto sendFileResult = PrinterNetworkManager::getInstance()->sendPrintFile(printerNetworkInfo, params);
    if (sendFileResult.isSuccess()) {
        wxLogMessage("File upload success: %s %s %s, file name: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                     printerNetworkInfo.printerModel, params.fileName.c_str());
        if (params.uploadAndStartPrint) {          
            auto sendTaskResult = PrinterNetworkManager::getInstance()->sendPrintTask(printerNetworkInfo, params);
            if (sendTaskResult.isError()) {
                wxLogWarning("Failed to send print task after file upload: %s %s %s, file name: %s, error: %s", printerNetworkInfo.host,
                             printerNetworkInfo.printerName, printerNetworkInfo.printerModel, params.fileName.c_str(),
                             sendTaskResult.message.c_str());
                if (params.errorFn) {
                    params.errorFn("Filed to start print, Error: " + std::to_string(static_cast<int>(sendTaskResult.code)) + " " +
                                   sendTaskResult.message);
                }
                return false;
            }
        }
        return true;
    } else {
        if (params.errorFn) {
            params.errorFn("Error: " + std::to_string(static_cast<int>(sendFileResult.code)) + " " + sendFileResult.message);
        }
        wxLogError("Failed to send print file: %s %s %s, file name: %s, error: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel, params.fileName.c_str(), sendFileResult.message.c_str());
    }
    return false;
}

void PrinterManager::savePrinterList()
{
    fs::path                    printerListPath = boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_list";
    std::ofstream               ofs(printerListPath.string(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(ofs);
    oarchive(mPrinterList);
}

void PrinterManager::updatePrinterConnectStatus(const std::string& printerId, const PrinterConnectStatus& status)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        it->second.connectStatus = status;
    }
}

void PrinterManager::updatePrinterStatus(const std::string& printerId, const PrinterStatus& status)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        it->second.printerStatus = status;
        if(status == PrinterStatus::PRINTER_STATUS_IDLE) {
            it->second.printTask.taskId = "";
            it->second.printTask.fileName = "";
            it->second.printTask.totalTime = 0;
            it->second.printTask.currentTime = 0;
            it->second.printTask.estimatedTime = 0;
            it->second.printTask.progress = 0;
        }
    }
}

void PrinterManager::updatePrinterPrintTask(const std::string& printerId, const PrinterPrintTask& task)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        it->second.printTask = task;
    }
}
void PrinterManager::updatePrinterAttributes(const std::string& printerId, const PrinterNetworkInfo& printerInfo)
{
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    auto                        it = mPrinterList.find(printerId);
    if (it != mPrinterList.end()) {
        if(printerInfo.firmwareVersion.empty()) {
            it->second.firmwareVersion = printerInfo.firmwareVersion;
        }
    }
}
} // namespace Slic3r
