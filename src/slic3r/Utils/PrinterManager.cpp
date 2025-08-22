#include "PrinterManager.hpp"
#include "slic3r/Utils/PrinterNetworkManager.hpp"

#include "libslic3r/PresetBundle.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "slic3r/Utils/PrintHost.hpp"
#include <boost/beast/core/detail/base64.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <wx/log.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include "PrinterCache.hpp"
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
    PrinterCache::getInstance()->loadPrinterList();
    PrinterNetworkManager::getInstance()->init();
}

PrinterManager::~PrinterManager() { close(); }

void PrinterManager::close() { 
    PrinterNetworkManager::getInstance()->close(); 
    PrinterCache::getInstance()->savePrinterList();
}

PrinterNetworkResult<bool> PrinterManager::deletePrinter(const std::string& printerId)
{
    auto printer = PrinterCache::getInstance()->getPrinter(printerId);
    if (!printer.has_value()) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, false);
    }
    PrinterCache::getInstance()->deletePrinter(printerId);
    PrinterCache::getInstance()->savePrinterList();
    wxLogMessage("Delete printer: %s %s %s", printer.value().host, printer.value().printerName, printer.value().printerModel);
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}
PrinterNetworkResult<bool> PrinterManager::updatePrinterName(const std::string& printerId, const std::string& printerName)
{
    auto printer = PrinterCache::getInstance()->getPrinter(printerId);
    if (!printer.has_value()) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, false);
    }
    PrinterCache::getInstance()->updatePrinterName(printerId, printerName);
    PrinterCache::getInstance()->savePrinterList();
    wxLogMessage("Update printer name: %s %s %s to %s", printer.value().host, printer.value().printerName, printer.value().printerModel, printerName);
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}
PrinterNetworkResult<bool> PrinterManager::updatePrinterHost(const std::string& printerId, const std::string& host)
{
    auto printer = PrinterCache::getInstance()->getPrinter(printerId);
    if (!printer.has_value()) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, false);
    }
    printer.value().host = host;
    PrinterCache::getInstance()->updatePrinterHost(printerId, printer.value());
    PrinterCache::getInstance()->savePrinterList();
    wxLogMessage("Update printer host: %s %s %s to %s", printer.value().host, printer.value().printerName, printer.value().printerModel, host);
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}
PrinterNetworkResult<bool> PrinterManager::addPrinter(PrinterNetworkInfo& printerNetworkInfo)
{
    // only generate a unique id for the printer when adding a printer
    // the printer info is from the UI, the UI info is from the discover device or manual add
    std::vector<PrinterNetworkInfo> printers = PrinterCache::getInstance()->getPrinters();
    for (const auto& p : printers) {
        if (p.host == printerNetworkInfo.host) {
            wxLogWarning("Printer already exists, host: %s, printerId: %s", printerNetworkInfo.host.c_str(), printerNetworkInfo.printerId.c_str());
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_ALREADY_EXISTS, false);
        }
    }
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printerNetworkInfo.addTime        = now;
    printerNetworkInfo.modifyTime     = now;
    printerNetworkInfo.lastActiveTime = now;
    printerNetworkInfo.printerId      = "";

    if(printerNetworkInfo.isPhysicalPrinter) {
        printerNetworkInfo.printerType = PrinterNetworkManager::getPrinterType(printerNetworkInfo);
        if(printerNetworkInfo.printerType == -1) {
            wxLogWarning("Failed to get device type for printer %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel);
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_TYPE_NOT_SUPPORTED, false);
        }
    }
    printerNetworkInfo.printerId = boost::uuids::to_string(boost::uuids::random_generator{}());

    std::shared_ptr<IPrinterNetwork> network;
    auto result = PrinterNetworkManager::getInstance()->connectToPrinter(printerNetworkInfo, network);
    if (result.isSuccess() && result.data.has_value()) {    
        auto attributes = network->getPrinterAttributes();
        if(attributes.isSuccess() && attributes.hasData()) {
            printerNetworkInfo.printerAttributes = attributes.data.value();
        } else {
            wxLogWarning("Failed to get printer attributes for added printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel, attributes.message.c_str());
            network->disconnectFromPrinter();
            return PrinterNetworkResult<bool>(attributes.isSuccess() ? PrinterNetworkErrorCode::PRINTER_RESPONSE_INVALID : attributes.code, false);
        }
        auto connectedPrinter = result.data.value();
        if(printerNetworkInfo.isPhysicalPrinter) {
            // update the printer network info
            printerNetworkInfo.mainboardId = connectedPrinter.mainboardId;
            printerNetworkInfo.serialNumber = connectedPrinter.serialNumber;
            printerNetworkInfo.printerType = connectedPrinter.printerType;
            printerNetworkInfo.webUrl        = connectedPrinter.webUrl;
            printerNetworkInfo.connectionUrl = connectedPrinter.connectionUrl;
            printerNetworkInfo.firmwareVersion = connectedPrinter.firmwareVersion;
            printerNetworkInfo.authMode = connectedPrinter.authMode;
            printerNetworkInfo.protocolVersion = connectedPrinter.protocolVersion;
        }
        printerNetworkInfo.connectStatus = PRINTER_CONNECT_STATUS_CONNECTED;
        PrinterNetworkManager::getInstance()->addPrinter(printerNetworkInfo, network);
        PrinterCache::getInstance()->addPrinter(printerNetworkInfo);
        PrinterCache::getInstance()->savePrinterList();
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
    } 
    wxLogWarning("Failed to add printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
        printerNetworkInfo.printerModel, result.message.c_str());
    return PrinterNetworkResult<bool>(result.isSuccess() ? PrinterNetworkErrorCode::PRINTER_RESPONSE_INVALID : result.code, false);          
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> PrinterManager::discoverPrinter()
{
    std::vector<PrinterNetworkInfo> printers;
    auto                            discoverResult = PrinterNetworkManager::getInstance()->discoverPrinters();
    if (!discoverResult.isSuccess()) {
        wxLogWarning("Failed to discover printers: %s", discoverResult.message.c_str());
        return discoverResult;
    }
    std::vector<PrinterNetworkInfo> discoverPrinterList;
    if (discoverResult.hasData()) {
        discoverPrinterList = discoverResult.data.value();
    }
    if (discoverPrinterList.empty()) {
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, printers);
    }

    auto vendorPrinterModelConfigMap = getVendorPrinterModelConfig();
    for (auto& discoverPrinter : discoverPrinterList) {
        // check if the device is already bound, if it is, check if the ip, firmware version, etc. have changed and update them
        bool isSamePrinter = false;
        for (auto& p : PrinterCache::getInstance()->getPrinters()) {
            if (!p.mainboardId.empty() && (discoverPrinter.mainboardId == p.mainboardId)) {
                isSamePrinter = true;
            }
            if (!p.serialNumber.empty() && (discoverPrinter.serialNumber == p.serialNumber)) {
                isSamePrinter = true;
            }
            if (isSamePrinter) {
                if (p.host != discoverPrinter.host) {
                    wxLogMessage("Printer %s %s %s IP changed, disconnect and connect to new IP, old IP: %s, new IP: %s", p.host,
                                 p.printerName, p.printerModel, p.host, discoverPrinter.host);
                    p.host          = discoverPrinter.host;
                    p.webUrl        = discoverPrinter.webUrl;
                    p.connectionUrl = discoverPrinter.connectionUrl;
                    PrinterCache::getInstance()->updatePrinterHost(p.printerId, p);
                    PrinterCache::getInstance()->savePrinterList();
                }
                break;
            }
        }
        if (isSamePrinter) {
            continue;
        }
        PrinterNetworkInfo printerNetworkInfo = discoverPrinter;
        printerNetworkInfo.printerId          = "";
        VendorProfile::PrinterModel printerModel;
        // update vendor, printerModel and hostType to keep consistent with the printer model in system preset
        VendorProfile machineProfile = getMachineProfile(discoverPrinter.vendor, discoverPrinter.printerModel, printerModel);
        printerNetworkInfo.vendor    = machineProfile.name;
        if (printerNetworkInfo.printerName.empty()) {
            printerNetworkInfo.printerName = printerModel.name;
        }
        printerNetworkInfo.printerModel = printerModel.name;
        if (printerNetworkInfo.printerType == 2) {
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
    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, printers);
}

std::vector<PrinterNetworkInfo> PrinterManager::getPrinterList() { return PrinterCache::getInstance()->getPrinters(); }
PrinterNetworkInfo PrinterManager::getPrinterNetworkInfo(const std::string& printerId)
{
    auto printer = PrinterCache::getInstance()->getPrinter(printerId);
    if (printer.has_value()) {
        return printer.value();
    } 
    wxLogError("Printer not found, printerId: %s", printerId.c_str());   
    return PrinterNetworkInfo();
}
PrinterNetworkResult<bool> PrinterManager::upload(PrinterNetworkParams& params)
{
    auto printer = PrinterCache::getInstance()->getPrinter(params.printerId);
    PrinterNetworkResult<bool> result(PrinterNetworkErrorCode::SUCCESS, false);
    if (!printer.has_value()) {
        wxLogError("Printer not found, File name: %s", params.fileName.c_str());
        result.code = PrinterNetworkErrorCode::PRINTER_NOT_FOUND;
    }
    else if (printer.value().connectStatus != PRINTER_CONNECT_STATUS_CONNECTED) {
        wxLogError("Printer not connected, File name: %s", params.fileName.c_str());
        result.code = PrinterNetworkErrorCode::PRINTER_OFFLINE;
    } else {
        result = PrinterNetworkManager::getInstance()->sendPrintFile(params);
        if (result.isSuccess()) {
            wxLogMessage("File upload success: %s %s %s, file name: %s", printer.value().host, printer.value().printerName,
                         printer.value().printerModel, params.fileName.c_str());
            if (params.uploadAndStartPrint) {
                result = PrinterNetworkManager::getInstance()->sendPrintTask(params);
                if (result.isError()) {
                    wxLogWarning("Failed to send print task after file upload: %s %s %s, file name: %s, error: %s", printer.value().host,
                                 printer.value().printerName, printer.value().printerModel, params.fileName.c_str(),
                                 result.message.c_str());
                }
            }
        }
    }
    if (params.errorFn) {
        params.errorFn("Error: " + std::to_string(static_cast<int>(result.code)) + " " + result.message);
    }
    if(result.isError()) {
        wxLogError("Failed to send print file: %s %s %s, file name: %s, error: %s", printer.value().host, printer.value().printerName,
                   printer.value().printerModel, params.fileName.c_str(), result.message.c_str());
    }
    return result;
}
PrinterNetworkResult<PrinterMmsGroup> PrinterManager::getPrinterMmsInfo(const std::string& printerId)
{
    auto printer = PrinterCache::getInstance()->getPrinter(printerId);
    if (printer.has_value()) {
        if (printer.value().connectStatus != PRINTER_CONNECT_STATUS_CONNECTED) {
            wxLogError("Printer not connected, printerId: %s", printerId.c_str());
            return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_OFFLINE, PrinterMmsGroup());
        }
        auto mmsInfo = PrinterNetworkManager::getInstance()->getPrinterMmsInfo(printer.value().printerId);
        if (mmsInfo.isSuccess() && mmsInfo.hasData()) {
            return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::SUCCESS, mmsInfo.data.value());
        } else {
            wxLogWarning("Failed to get printer mms info: %s %s %s, error: %s", printer.value().host, printer.value().printerName,
                         printer.value().printerModel, mmsInfo.message.c_str());
            return PrinterNetworkResult<PrinterMmsGroup>(mmsInfo.isSuccess() ? PrinterNetworkErrorCode::PRINTER_RESPONSE_INVALID :
                                                                               mmsInfo.code,
                                                         PrinterMmsGroup());
        }
    }
    wxLogWarning("Printer not found, printerId: %s", printerId.c_str());
    return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, PrinterMmsGroup());
}
} // namespace Slic3r
