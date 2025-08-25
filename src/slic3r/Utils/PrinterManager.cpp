#include "PrinterManager.hpp"
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
#include <thread>
#include <future>
#include <mutex>
#include <algorithm>
#include "PrinterCache.hpp"
#include "PrinterNetworkEvent.hpp"
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
PrinterManager::PrinterManager(){ }
PrinterManager::~PrinterManager() { }

void PrinterManager::init() {
    IPrinterNetwork::init();
    // connect status changed event
    PrinterNetworkEvent::getInstance()->connectStatusChanged.connect([this](const PrinterConnectStatusEvent& event) {
        PrinterCache::getInstance()->updatePrinterConnectStatus(event.printerId, event.status);
    });

    // printer status changed event
    PrinterNetworkEvent::getInstance()->statusChanged.connect(
        [this](const PrinterStatusEvent& event) { PrinterCache::getInstance()->updatePrinterStatus(event.printerId, event.status); });

    // printer print task changed event
    PrinterNetworkEvent::getInstance()->printTaskChanged.connect(
        [this](const PrinterPrintTaskEvent& event) { PrinterCache::getInstance()->updatePrinterPrintTask(event.printerId, event.task); });

    // printer attributes changed event
    PrinterNetworkEvent::getInstance()->attributesChanged.connect([this](const PrinterAttributesEvent& event) {
        PrinterCache::getInstance()->updatePrinterAttributes(event.printerId, event.printerInfo);
    });

    mIsRunning        = true;
    mConnectionThread = std::thread([this]() { monitorPrinterConnections(); });

    PrinterCache::getInstance()->loadPrinterList();
}

void PrinterManager::close() { 

    if(!mIsRunning) {
        return;
    }
    mIsRunning = false;
    if (mConnectionThread.joinable()) {
        mConnectionThread.join();
    }
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    for (const auto& [printerId, network] : mNetworkConnections) {
        network->disconnectFromPrinter();
    }
    mNetworkConnections.clear();
    PrinterCache::getInstance()->savePrinterList();
    IPrinterNetwork::uninit();  
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
    std::vector<PrinterNetworkInfo> printers = PrinterCache::getInstance()->getPrinters();
    for (auto& p : printers) {
        if (p.host == host && p.printerId != printerId) {
            wxLogWarning("Printer already exists, host: %s, printerId: %s", p.host, printerId);
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_ALREADY_EXISTS, false);
        }
    }
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
        if (p.host == printerNetworkInfo.host || 
            (!p.serialNumber.empty() && p.serialNumber == printerNetworkInfo.serialNumber) || 
            (!p.mainboardId.empty() && p.mainboardId == printerNetworkInfo.mainboardId)) {
            wxLogWarning("Printer already exists, host: %s, serialNumber: %s, mainboardId: %s", printerNetworkInfo.host.c_str(), printerNetworkInfo.serialNumber.c_str(), printerNetworkInfo.mainboardId.c_str());
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_ALREADY_EXISTS, false);
        }
    }
    
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printerNetworkInfo.addTime        = now;
    printerNetworkInfo.modifyTime     = now;
    printerNetworkInfo.lastActiveTime = now;
    printerNetworkInfo.printerId      = "";

    if(printerNetworkInfo.isPhysicalPrinter) {
        printerNetworkInfo.printerType = getPrinterType(printerNetworkInfo);
        if(printerNetworkInfo.printerType == -1) {
            wxLogWarning("Failed to get device type for printer %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel);
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_TYPE_NOT_SUPPORTED, false);
        }
    }

    printerNetworkInfo.printerId = boost::uuids::to_string(boost::uuids::random_generator{}());

    std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
    if (!network) {
        wxLogError("Failed to create network for printer, host: %s, printerName: %s, printerModel: %s, hostType: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                   printerNetworkInfo.printerModel, printerNetworkInfo.hostType);
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::CREATE_NETWORK_FOR_HOST_TYPE_FAILED, false);
    }
    auto connectResult = network->connectToPrinter();
    if (connectResult.isSuccess() && connectResult.data.has_value()) {    
        auto attributes = network->getPrinterAttributes();
        if(attributes.isSuccess() && attributes.hasData()) {
            printerNetworkInfo.printerAttributes = attributes.data.value();
        } else {
            wxLogWarning("Failed to get printer attributes for added printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel, attributes.message.c_str());
            network->disconnectFromPrinter();
            return PrinterNetworkResult<bool>(attributes.isSuccess() ? PrinterNetworkErrorCode::PRINTER_RESPONSE_INVALID : attributes.code, false);
        }
        auto connectedPrinter = connectResult.data.value();
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
        addPrinterNetwork(network);
        PrinterCache::getInstance()->addPrinter(printerNetworkInfo);
        PrinterCache::getInstance()->savePrinterList();
        wxLogMessage("Added printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName, printerNetworkInfo.printerModel);
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
    } 
    wxLogWarning("Failed to add printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
        printerNetworkInfo.printerModel, connectResult.message.c_str());
    return PrinterNetworkResult<bool>(connectResult.isSuccess() ? PrinterNetworkErrorCode::PRINTER_RESPONSE_INVALID : connectResult.code, false);          
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> PrinterManager::discoverPrinter()
{
    std::vector<PrinterNetworkInfo> discoveredPrinters;
    for (const auto& printerHostType : {htElegooLink, htOctoPrint, htPrusaLink, htPrusaConnect, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS,
                                  htESP3D, htCrealityPrint, htObico, htFlashforge, htSimplyPrint}) {
        PrinterNetworkInfo printerNetworkInfo;
        printerNetworkInfo.hostType = PrintHost::get_print_host_type_str(printerHostType);
        std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
        if(!network) {
            continue;
        }
        auto result = network->discoverPrinters();
        if (result.isSuccess() && result.hasData()) {
            discoveredPrinters.insert(discoveredPrinters.end(), result.data.value().begin(), result.data.value().end());
        } else if (result.isError()) {
            wxLogWarning("Failed to discover devices for host type %d: %s", static_cast<int>(printerHostType), result.message.c_str());
        }
    }
    
    std::vector<PrinterNetworkInfo> printersToAdd;
    auto vendorPrinterModelConfigMap = getVendorPrinterModelConfig(); 
    for (auto& discoveredPrinter : discoveredPrinters) {
        // check if the device is already bound, if it is, check if the ip, firmware version, etc. have changed and update them
        bool isSamePrinter = false;
        for (auto& p : PrinterCache::getInstance()->getPrinters()) {
            if (!p.mainboardId.empty() && (discoveredPrinter.mainboardId == p.mainboardId)) {
                isSamePrinter = true;
            }
            if (!p.serialNumber.empty() && (discoveredPrinter.serialNumber == p.serialNumber)) {
                isSamePrinter = true;
            }
            if (isSamePrinter) {
                if (p.host != discoveredPrinter.host) {
                    wxLogMessage("Printer %s %s %s IP changed, disconnect and connect to new IP, old IP: %s, new IP: %s", p.host,
                                 p.printerName, p.printerModel, p.host, discoveredPrinter.host);
                    p.host          = discoveredPrinter.host;
                    p.webUrl        = discoveredPrinter.webUrl;
                    p.connectionUrl = discoveredPrinter.connectionUrl;
                    PrinterCache::getInstance()->updatePrinterHost(p.printerId, p);
                    PrinterCache::getInstance()->savePrinterList();
                }
                break;
            }
        }
        if (isSamePrinter) {
            continue;
        }
        PrinterNetworkInfo printerNetworkInfo = discoveredPrinter;
        printerNetworkInfo.printerId          = "";
        VendorProfile::PrinterModel printerModel;
        // update vendor, printerModel and hostType to keep consistent with the printer model in system preset
        VendorProfile machineProfile = getMachineProfile(discoveredPrinter.vendor, discoveredPrinter.printerModel, printerModel);
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
        printersToAdd.push_back(printerNetworkInfo);
    }
    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, printersToAdd);
}

std::vector<PrinterNetworkInfo> PrinterManager::getPrinterList() {
    auto printers = PrinterCache::getInstance()->getPrinters();
    std::sort(printers.begin(), printers.end(),
    [](const PrinterNetworkInfo& a, const PrinterNetworkInfo& b) { return a.addTime < b.addTime; });
    return printers;
}

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
    bool isSendPrintTaskFailed = false;

    do {
        if (!printer.has_value()) {
            wxLogError("Printer not found, File name: %s", params.fileName.c_str());
            result.code = PrinterNetworkErrorCode::PRINTER_NOT_FOUND;
            break;
        }
        if (printer.value().connectStatus != PRINTER_CONNECT_STATUS_CONNECTED) {
            wxLogError("Printer not connected, File name: %s", params.fileName.c_str());
            result.code = PrinterNetworkErrorCode::PRINTER_OFFLINE;
            break;
        }
        std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(params.printerId);
        if(!network) {
            wxLogError("No network connection for printer: %s", params.printerId.c_str());
            result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_FOUND, false);
            break;
        }
        result = network->sendPrintFile(params);
        if (result.isError()) {
            wxLogError("Failed to send print file to printer %s %s %s %s", network->getPrinterNetworkInfo().host, network->getPrinterNetworkInfo().printerName,
                       network->getPrinterNetworkInfo().printerModel, result.message.c_str());
            break;
        }
        if (result.isSuccess()) {
            wxLogMessage("File upload success: %s %s %s, file name: %s", printer.value().host, printer.value().printerName,
                         printer.value().printerModel, params.fileName.c_str());
            if (params.uploadAndStartPrint) {
                result = network->sendPrintTask(params);
                if(result.isError()) {
                    isSendPrintTaskFailed = true;
                }
            }
        }
    } while (0);

    if(result.isError()) {
        if(isSendPrintTaskFailed) {
            wxLogError("Failed to send print task after file upload: %s %s %s, file name: %s, error: %s", printer.value().host, printer.value().printerName,
                       printer.value().printerModel, params.fileName.c_str(), result.message.c_str());
        } else {
            wxLogError("Failed to send print file: %s %s %s, file name: %s, error: %s", printer.value().host, printer.value().printerName,
                       printer.value().printerModel, params.fileName.c_str(), result.message.c_str());
        }
        if (params.errorFn) {
            std::string errorMessage = isSendPrintTaskFailed ? "Send print task failed" : "Send print file failed";
            params.errorFn(errorMessage + "(" + std::to_string(static_cast<int>(result.code)) + ")" + result.message);
        }
    }
    return result;
}

PrinterNetworkResult<PrinterMmsGroup> PrinterManager::getPrinterMmsInfo(const std::string& printerId)
{
    auto printer = PrinterCache::getInstance()->getPrinter(printerId);
    if (!printer.has_value()) {
        wxLogError("Printer not found, printerId: %s", printerId.c_str());
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, PrinterMmsGroup());
    }
    if (printer.value().connectStatus != PRINTER_CONNECT_STATUS_CONNECTED) {
        wxLogError("Printer not connected, printerId: %s", printerId.c_str());
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_OFFLINE, PrinterMmsGroup());
    }

    std::shared_ptr<IPrinterNetwork>  network = getPrinterNetwork(printerId);
    if (!network) {
        wxLogError("No network connection for printer: %s", printerId.c_str());
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::NOT_FOUND, PrinterMmsGroup());
    }
    PrinterNetworkResult<PrinterMmsGroup>   result = network->getPrinterMmsInfo();

    if (result.isSuccess() && result.hasData()) {
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::SUCCESS, result.data.value());
    }
    wxLogWarning("Failed to get printer mms info: %s %s %s, error: %s", printer.value().host, printer.value().printerName,
                 printer.value().printerModel, result.message.c_str());
    return PrinterNetworkResult<PrinterMmsGroup>(result.isSuccess() ? result.code : PrinterNetworkErrorCode::PRINTER_RESPONSE_INVALID, PrinterMmsGroup());
}


void PrinterManager::monitorPrinterConnections()
{
    while (mIsRunning) {
        auto printerList = PrinterCache::getInstance()->getPrinters();
        std::vector<std::future<void>> connectionFutures;
        
        for (const auto& printer : printerList) {
            std::shared_ptr<IPrinterNetwork> activeNetwork = getPrinterNetwork(printer.printerId);
            if (activeNetwork && printer.connectStatus == PRINTER_CONNECT_STATUS_CONNECTED && activeNetwork->getPrinterNetworkInfo().host == printer.host) {
                // already connected and the host is the same, no need to connect again
                continue;
            }
            bool ipChanged = false;
            if(activeNetwork && activeNetwork->getPrinterNetworkInfo().host != printer.host) {
                ipChanged = true;
            }
            
            auto future = std::async(std::launch::async, [this, printer, ipChanged]() {
                if(printer.connectStatus == PRINTER_CONNECT_STATUS_CONNECTED || ipChanged) {
                    PrinterCache::getInstance()->updatePrinterConnectStatus(printer.printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
                    deletePrinterNetwork(printer.printerId);
                }
                std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printer);
                if(!network) {
                    wxLogError("Failed to create network for printer: %s %s %s", printer.host, printer.printerName, printer.printerModel);
                    return;
                }
                auto connectResult = network->connectToPrinter();
                if (connectResult.isSuccess()) {
                    addPrinterNetwork(network);
                    PrinterCache::getInstance()->updatePrinterConnectStatus(printer.printerId, PRINTER_CONNECT_STATUS_CONNECTED);
                } 
            });
            connectionFutures.push_back(std::move(future));
        }
        
        for (auto& future : connectionFutures) {
            future.wait();
        }
        
        // disconnect from printers that are not in the printer list, mutex lock is needed
        std::vector<std::string> printerIdsToRemove;
        {
            std::lock_guard<std::mutex> lock(mConnectionsMutex);
            for (const auto& [printerId, network] : mNetworkConnections) {
                auto printer = PrinterCache::getInstance()->getPrinter(printerId);
                if (!printer.has_value() || printer.value().host != network->getPrinterNetworkInfo().host) {
                    printerIdsToRemove.push_back(printerId);
                    wxLogWarning("Printer not in list, disconnect from printer: %s %s %s", network->getPrinterNetworkInfo().host,
                                 network->getPrinterNetworkInfo().printerName, network->getPrinterNetworkInfo().printerModel);
                }
            }
        }
        
        for (const auto& printerId : printerIdsToRemove) {
            deletePrinterNetwork(printerId);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool PrinterManager::addPrinterNetwork(const std::shared_ptr<IPrinterNetwork>& network)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    mNetworkConnections[network->getPrinterNetworkInfo().printerId] = network;
    return true;
}
bool PrinterManager::deletePrinterNetwork(const std::string& printerId)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto it = mNetworkConnections.find(printerId);
    if(it == mNetworkConnections.end()) {
        return false;
    }
    auto network = it->second;
    network->disconnectFromPrinter();
    mNetworkConnections.erase(it);
    return true;
}

std::shared_ptr<IPrinterNetwork> PrinterManager::getPrinterNetwork(const std::string& printerId)
{
    std::lock_guard<std::mutex> lock(mConnectionsMutex);
    auto it = mNetworkConnections.find(printerId);

    if (it != mNetworkConnections.end()) {
        return it->second;
    }
    return nullptr;
}

int PrinterManager::getPrinterType(const PrinterNetworkInfo& printerNetworkInfo)
{
    auto network = PrinterNetworkFactory::createNetwork(printerNetworkInfo);
    if (network) {
        return network->getPrinterType();
    }
    return -1;
}

} // namespace Slic3r
