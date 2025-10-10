#include "PrinterManager.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include <wx/app.h>
#include <wx/wx.h>
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
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/Utils/PrinterPluginManager.hpp"
namespace Slic3r {


 // Static member definitions for PrinterLock
std::map<std::string, std::mutex> PrinterManager::PrinterLock::sPrinterMutexes;
std::mutex PrinterManager::PrinterLock::sMutex;

PrinterManager::PrinterLock::PrinterLock(const std::string& printerId) : mPrinterMutex(nullptr) {
    {
        std::unique_lock<std::mutex> lock(sMutex);
        auto it = sPrinterMutexes.find(printerId);
        if(it == sPrinterMutexes.end()) {
            it = sPrinterMutexes.try_emplace(printerId).first;
        }
        mPrinterMutex = &(it->second);
    }
    
    if (mPrinterMutex) {
        mPrinterMutex->lock();
    }
}

PrinterManager::PrinterLock::~PrinterLock() {
    if (mPrinterMutex) {
        mPrinterMutex->unlock();
    }
}

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
        // Printer connection status change handled by PrinterCache
    });

    // printer status changed event
    PrinterNetworkEvent::getInstance()->statusChanged.connect(
        [this](const PrinterStatusEvent& event) { 
            PrinterCache::getInstance()->updatePrinterStatus(event.printerId, event.status);
        });

    // printer print task changed event
    PrinterNetworkEvent::getInstance()->printTaskChanged.connect(
        [this](const PrinterPrintTaskEvent& event) { 
            PrinterCache::getInstance()->updatePrinterPrintTask(event.printerId, event.task);
        });

    // printer attributes changed event
    PrinterNetworkEvent::getInstance()->attributesChanged.connect([this](const PrinterAttributesEvent& event) {
        PrinterCache::getInstance()->updatePrinterAttributesByNotify(event.printerId, event.printerInfo);
    });


    mIsRunning        = true;
    mConnectionThread = std::thread([this]() { monitorPrinterConnections(); });

    PrinterCache::getInstance()->loadPrinterList();

    syncOldPresetPrinters();

    PrinterPluginManager::getInstance()->init();

    NetworkUserInfo userInfo;
    userInfo.userId = 55;
    userInfo.username = "";
    userInfo.token = "45f234d84b4c14546fe0487bbbe77015";
    userInfo.refreshToken = "c891b3a4730edb86f26133708321e11d";
    userInfo.accessTokenExpireTime = 1774681687887;
    userInfo.refreshTokenExpireTime = 1883545687887;
    userInfo.hostType = "ElegooLink";

    loginWAN(userInfo);
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
    
    // Printer deletion notification removed as requested

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
    PrinterLock lock(printerId);
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
    // delete the printer network connection
    PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
    deletePrinterNetwork(printerId);

    printer.value().host = host;
    std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printer.value());
    if (!network) {
        wxLogError("Failed to create network for printer, host: %s, printerName: %s, printerModel: %s, hostType: %s", printer.value().host, printer.value().printerName,
                   printer.value().printerModel, printer.value().hostType);
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::CREATE_NETWORK_FOR_HOST_TYPE_FAILED, false);
    }
    auto connectResult = network->connectToPrinter();
    if (connectResult.isSuccess()) {    
        auto attributes = network->getPrinterAttributes();
        if(attributes.isSuccess() && attributes.hasData()) {
            auto printerAttributes = attributes.data.value();   
            if(printerAttributes.mainboardId != printer.value().mainboardId || printerAttributes.serialNumber != printer.value().serialNumber) {
                wxLogError("Printer mainboardId or serialNumber not match, local: %s, %s, remote: %s, %s", printer.value().mainboardId.c_str(), printer.value().serialNumber.c_str(), printerAttributes.mainboardId.c_str(), printerAttributes.serialNumber.c_str());
                network->disconnectFromPrinter();
                return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_HOST_NOT_MATCH, false);
            }
            PrinterCache::getInstance()->updatePrinterHost(printerId, printer.value());
            PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_CONNECTED);
            PrinterCache::getInstance()->updatePrinterAttributes(printerId, printerAttributes);
            addPrinterNetwork(network);
            PrinterCache::getInstance()->savePrinterList();
            wxLogMessage("Update printer host: %s %s %s to %s", printer.value().host, printer.value().printerName, printer.value().printerModel, host);
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);

        } else {
            wxLogWarning("Failed to get printer attributes for added printer %s %s %s: %s", printer.value().host, printer.value().printerName,
                         printer.value().printerModel, attributes.message.c_str());
            network->disconnectFromPrinter();
            return PrinterNetworkResult<bool>(attributes.isSuccess() ? PrinterNetworkErrorCode::PRINTER_INVALID_RESPONSE : attributes.code,false);
        }
    }
    wxLogWarning("Failed to connect to printer, host: %s, printerName: %s, printerModel: %s, hostType: %s", printer.value().host, printer.value().printerName,
                 printer.value().printerModel, printer.value().hostType);
    return PrinterNetworkResult<bool>(connectResult.code, false);
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
            auto printerAttributes = attributes.data.value();   
            printerNetworkInfo.printCapabilities = printerAttributes.printCapabilities;
            printerNetworkInfo.systemCapabilities = printerAttributes.systemCapabilities;
            printerNetworkInfo.firmwareVersion = printerAttributes.firmwareVersion;
            printerNetworkInfo.mainboardId = printerAttributes.mainboardId;
            printerNetworkInfo.serialNumber = printerAttributes.serialNumber;
            printerNetworkInfo.webUrl = printerAttributes.webUrl;
            if(printerNetworkInfo.isPhysicalPrinter) {
                printerNetworkInfo.authMode = printerAttributes.authMode;
            }
        } else {
            wxLogWarning("Failed to get printer attributes for added printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
                         printerNetworkInfo.printerModel, attributes.message.c_str());
            network->disconnectFromPrinter();
            return PrinterNetworkResult<bool>(attributes.isSuccess() ? PrinterNetworkErrorCode::PRINTER_INVALID_RESPONSE : attributes.code,false);
        }
        printerNetworkInfo.connectStatus = PRINTER_CONNECT_STATUS_CONNECTED;
        addPrinterNetwork(network);
        PrinterCache::getInstance()->addPrinter(printerNetworkInfo);
        PrinterCache::getInstance()->savePrinterList();
        wxLogMessage("Added printer: %s %s %s", printerNetworkInfo.host, printerNetworkInfo.printerName, printerNetworkInfo.printerModel);
        
        // Printer addition notification removed as requested
        
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
    } 
    wxLogWarning("Failed to add printer %s %s %s: %s", printerNetworkInfo.host, printerNetworkInfo.printerName,
        printerNetworkInfo.printerModel, connectResult.message.c_str());
    return PrinterNetworkResult<bool>(connectResult.isSuccess() ? PrinterNetworkErrorCode::PRINTER_INVALID_RESPONSE : connectResult.code,false);
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
            result.code = PrinterNetworkErrorCode::PRINTER_CONNECTION_ERROR;
            break;
        }
        std::shared_ptr<IPrinterNetwork> network = getPrinterNetwork(params.printerId);
        if(!network) {
            wxLogError("No network connection for printer: %s", params.printerId.c_str());
            result = PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, false);
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
            std::string errorMessage = isSendPrintTaskFailed ? _u8L("Send print task failed") : _u8L("Send print file failed");
            params.errorFn(errorMessage + ", " + result.message);
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
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_CONNECTION_ERROR, PrinterMmsGroup());
    }

    std::shared_ptr<IPrinterNetwork>  network = getPrinterNetwork(printerId);
    if (!network) {
        wxLogError("No network connection for printer: %s", printerId.c_str());
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_NOT_FOUND, PrinterMmsGroup());
    }
    PrinterNetworkResult<PrinterMmsGroup>  result = network->getPrinterMmsInfo();
      
    if (result.isSuccess() && result.hasData()) {
        if(!result.data.value().connected) {
            std::string mmsSystemName = "MMS";
            if(!result.data.value().mmsSystemName.empty()) {
               mmsSystemName = result.data.value().mmsSystemName;
            }
            std::string errorMessage = (boost::format(_u8L("%1% connection failed. Please check and try again.")) % mmsSystemName).str();
            return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::PRINTER_MMS_NOT_CONNECTED, PrinterMmsGroup(), errorMessage);
        }
        return PrinterNetworkResult<PrinterMmsGroup>(PrinterNetworkErrorCode::SUCCESS, result.data.value());
    }

    wxLogWarning("Failed to get printer mms info: %s %s %s, error: %s", printer.value().host, printer.value().printerName,
    printer.value().printerModel, result.message.c_str());
    return PrinterNetworkResult<PrinterMmsGroup>(result.isSuccess() ? PrinterNetworkErrorCode::PRINTER_INVALID_RESPONSE : result.code,
                                                 PrinterMmsGroup());
}


void PrinterManager::monitorPrinterConnections()
{
    while (mIsRunning) {
        auto printerList = PrinterCache::getInstance()->getPrinters();
        std::vector<std::future<void>> connectionFutures;
        
        for (const auto& printer : printerList) {
            std::string printerId = printer.printerId;
            auto future = std::async(std::launch::async, [this, printerId]() {
                // lock the printer
                PrinterLock lock(printerId);
                // because the printer info may be modified during the lock, so we need to get the printer again
                auto printer = PrinterCache::getInstance()->getPrinter(printerId);
                if (!printer.has_value()) {
                    wxLogError("Printer not found, printerId: %s", printerId.c_str());
                    return;
                }

                std::shared_ptr<IPrinterNetwork> activeNetwork = getPrinterNetwork(printerId);
                
                if(printer.value().printerStatus == PRINTER_STATUS_ID_NOT_MATCH || printer.value().printerStatus == PRINTER_STATUS_AUTH_ERROR) {                   
                    return;
                }

                if (activeNetwork && printer.value().connectStatus == PRINTER_CONNECT_STATUS_CONNECTED && activeNetwork->getPrinterNetworkInfo().host == printer.value().host) {
                    // already connected and the host is the same, no need to connect again
                    return;
                }

                bool ipChanged = false;
                if(activeNetwork && activeNetwork->getPrinterNetworkInfo().host != printer.value().host) {
                    ipChanged = true;
                }

                if(printer.value().connectStatus == PRINTER_CONNECT_STATUS_CONNECTED || ipChanged) {
                    PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
                    deletePrinterNetwork(printerId);
                }
                std::shared_ptr<IPrinterNetwork> network = PrinterNetworkFactory::createNetwork(printer.value());
                if(!network) {
                    wxLogError("Failed to create network for printer: %s %s %s", printer.value().host, printer.value().printerName, printer.value().printerModel);
                    return;
                }
                auto connectResult = network->connectToPrinter();
                if (connectResult.isSuccess()) {
                    auto attributes = network->getPrinterAttributes();
                    if(attributes.isSuccess() && attributes.hasData()) {
                        // update printer attributes
                        auto printerAttributes = attributes.data.value();
                        if(!printer.value().mainboardId.empty() && !printerAttributes.mainboardId.empty() && printer.value().mainboardId != printerAttributes.mainboardId) {
                            // the mainboardId of the local printer is not the same as the remote printer, which means the remote printer has been replaced with a new mainboard
                            // need to delete the printer and add the printer or discover the printer again
                            wxLogError("Printer mainboardId not match, local: %s, remote: %s", printer.value().mainboardId.c_str(), printerAttributes.mainboardId.c_str());
                            PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
                            PrinterCache::getInstance()->updatePrinterStatus(printerId, PRINTER_STATUS_ID_NOT_MATCH);
                            deletePrinterNetwork(printerId);
                            return;
                        }
                        if(!printer.value().serialNumber.empty() && !printerAttributes.serialNumber.empty() && printer.value().serialNumber != printerAttributes.serialNumber) {    
                            // the serialNumber of the local printer is not the same as the remote printer, which means the remote printer has been replaced with a new serial number
                            //need to delete the printer and  add the printer or discover the printer again
                            wxLogError("Printer serialNumber not match, local: %s, remote: %s", printer.value().serialNumber.c_str(), printerAttributes.serialNumber.c_str());
                            PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
                            PrinterCache::getInstance()->updatePrinterStatus(printerId, PRINTER_STATUS_ID_NOT_MATCH);
                            deletePrinterNetwork(printerId);
                            return;
                        }
                        PrinterCache::getInstance()->updatePrinterAttributes(printerId, printerAttributes);
                        addPrinterNetwork(network);
                        PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_CONNECTED);
                        wxLogMessage("Connected to printer: %s %s %s, firmware version: %s", printer.value().host, printer.value().printerName, printer.value().printerModel, printerAttributes.firmwareVersion.c_str());
                    } else {
                        wxLogError("Failed to get printer attributes for printer: %s %s %s", printer.value().host, printer.value().printerName, printer.value().printerModel);
                    }
                } else {

                    if(connectResult.code == PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD || 
                        connectResult.code == PrinterNetworkErrorCode::INVALID_TOKEN || 
                        connectResult.code == PrinterNetworkErrorCode::INVALID_ACCESS_CODE || 
                        connectResult.code == PrinterNetworkErrorCode::INVALID_PIN_CODE) {
                        PrinterCache::getInstance()->updatePrinterStatus(printerId, PRINTER_STATUS_AUTH_ERROR);                              
                    }
                    wxLogError("Failed to connect to printer: %s %s %s, error: %s", printer.value().host, printer.value().printerName, printer.value().printerModel, connectResult.message.c_str());
                    PrinterCache::getInstance()->updatePrinterConnectStatus(printerId, PRINTER_CONNECT_STATUS_DISCONNECTED);
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
            PrinterLock lock(printerId);
            deletePrinterNetwork(printerId);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
// first get selected printer by modelName and printerId
// if not found, get selected printer by modelName
// if not found, get selected printer by printerId
// if not found, return first printer
PrinterNetworkInfo PrinterManager::getSelectedPrinter(const std::string &printerModel, const std::string &printerId)
{
    auto printers = getPrinterList();
    PrinterNetworkInfo selectedPrinter;
    if (!printerModel.empty() && !printerId.empty()) {
        for (auto& printer : printers) {
            if (printer.printerModel == printerModel && printer.printerId == printerId) {
                selectedPrinter = printer;
                break;
            }
        }
    }
    if (!printerModel.empty() && selectedPrinter.printerId.empty()) {
        for (auto& printer : printers) {
            if (printer.printerModel == printerModel) {
                selectedPrinter = printer;
                break;
            }
        }
    }
    if (!printerId.empty() && selectedPrinter.printerId.empty()) {
        for (auto& printer : printers) {
            if (printer.printerId == printerId) {
                selectedPrinter = printer;
                break;
            }
        }
    }
    if (selectedPrinter.printerId.empty() && !printers.empty()) {
        selectedPrinter = printers[0];
    }
    return selectedPrinter;
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

void PrinterManager::syncOldPresetPrinters()
{
    // Check if migration has already been completed
    if (wxGetApp().app_config->get("machine_migration_completed") == "1") {
        return;
    }
    
    try {
        if (!wxGetApp().preset_bundle) {
            wxLogMessage("Preset bundle not available, skipping sync");
            return;
        }
        const auto& printerPresets = wxGetApp().preset_bundle->printers;

        for (const auto& preset : printerPresets) {
            if (preset.is_system) {
                continue;
            }
            const auto& config = preset.config;
            if (preset.name.empty() || !config.has("host_type") || !config.has("print_host") || !config.has("printer_model")) {
                continue;
            }

            PrinterNetworkInfo printerInfo;
            printerInfo.isPhysicalPrinter = true;
            printerInfo.printerName       = preset.name;

            auto printerModel = config.option<ConfigOptionString>("printer_model")->value;
            
            // Find vendor by printer model
            std::string vendorName;
            for (const auto& vendorProfile : wxGetApp().preset_bundle->vendors) {
                for (const auto& vendorModel : vendorProfile.second.models) {
                    if (vendorModel.name == printerModel) {
                        vendorName = vendorProfile.first;
                        break;
                    }
                }
                if (!vendorName.empty()) break;
            }
            if(vendorName.empty()) {
                continue;
            }
            printerInfo.vendor = vendorName;
            printerInfo.printerModel = printerModel;
            auto host         = config.option<ConfigOptionString>("print_host");
            if(host && !host->value.empty()) {
                printerInfo.host = host->value;
            } else {
                continue;
            }
            auto hostType     = config.option<ConfigOptionEnum<PrintHostType>>("host_type");
            if(!PrintHost::support_device_list_management(config)) {
                continue;
            }
            if(hostType) {
                std::string hostTypeStr = PrintHost::get_print_host_type_str(hostType->value);
                if (!hostTypeStr.empty()) {
                    printerInfo.hostType = hostTypeStr;
                } else {
                    continue;
                }
            } else {
                continue;
            }
            if (config.has("printhost_port")) {
                auto port = config.option<ConfigOptionString>("printhost_port");
                if(port && !port->value.empty()) {
                    printerInfo.port = std::stoi(port->value);
                }
            }
            if (config.has("print_host_webui")) {
                auto print_host_webui = config.option<ConfigOptionString>("print_host_webui");
                if(print_host_webui && !print_host_webui->value.empty()) {
                    printerInfo.webUrl = print_host_webui->value;
                }
            }

            nlohmann::json extraInfo = nlohmann::json();
            if (config.has("printhost_apikey")) {
                auto printhost_apikey = config.option<ConfigOptionString>("printhost_apikey");
                if(printhost_apikey && !printhost_apikey->value.empty()) {
                    extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_VENDOR] = printhost_apikey->value;
                }
            }
            if (config.has("printhost_cafile")) {
                auto cafile = config.option<ConfigOptionString>("printhost_cafile");
                if(cafile && !cafile->value.empty()) {
                    extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_PORT] = cafile->value;
                }
            }
            if (config.has("printhost_ssl_ignore_revoke")) {
                auto sslIgnoreRevoke = config.option<ConfigOptionString>("printhost_ssl_ignore_revoke");
                if(sslIgnoreRevoke && !sslIgnoreRevoke->value.empty()) {
                    extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_HOST] = sslIgnoreRevoke->value;
                }
            }
            printerInfo.extraInfo = extraInfo.dump();

            bool alreadyExists = false;
            auto printers = PrinterCache::getInstance()->getPrinters();
            for(const auto& printer : printers) {
                if(printer.host == printerInfo.host) {
                    alreadyExists = true;
                    break;
                }
            }
            if(alreadyExists) {
                continue;
            }
            // Generate unique printerId
            boost::uuids::uuid uuid = boost::uuids::random_generator()();
            printerInfo.printerId = boost::uuids::to_string(uuid);
            
            // Set timestamps
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            printerInfo.addTime = timestamp;
            printerInfo.modifyTime = timestamp;
            printerInfo.lastActiveTime = timestamp;

            PrinterCache::getInstance()->addPrinter(printerInfo);
        }

        wxLogMessage("Preset printers synced successfully");
        
        // Mark migration as completed
        wxGetApp().app_config->set("machine_migration_completed", "1");
        wxGetApp().app_config->save();
        
    } catch (const std::exception& e) {
        wxLogMessage("Error syncing preset printers: %s", e.what());
    }
}

PrinterNetworkResult<bool> PrinterManager::loginWAN(const NetworkUserInfo& userInfo)
{
    PrinterNetworkInfo printerInfo;
    printerInfo.hostType = userInfo.hostType;

    std::shared_ptr<IPrinterNetwork> work = PrinterNetworkFactory::createNetwork(printerInfo);
    if(!work) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
    }
    mNetworkWAN = work;
    auto result = mNetworkWAN->loginWAN(userInfo);
    if(result.isSuccess()) {
        mNetworkUserInfo = userInfo;
    }
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}

PrinterNetworkResult<PrinterPrintFileResponse> PrinterManager::getFileList(const std::string& printerId, int pageNumber, int pageSize)
{
    PrinterPrintFileResponse printFiles;
    if(!mNetworkWAN) {
        return PrinterNetworkResult<PrinterPrintFileResponse>(PrinterNetworkErrorCode::SUCCESS, printFiles);
    }
    return mNetworkWAN->getFileList(printerId, pageNumber, pageSize);
}
PrinterNetworkResult<PrinterPrintTaskResponse> PrinterManager::getPrintTaskList(const std::string& printerId, int pageNumber, int pageSize)
{
    PrinterPrintTaskResponse printTasks;
    if(!mNetworkWAN) {
        return PrinterNetworkResult<PrinterPrintTaskResponse>(PrinterNetworkErrorCode::SUCCESS, printTasks);
    }
    return mNetworkWAN->getPrintTaskList(printerId, pageNumber, pageSize);
}
PrinterNetworkResult<bool> PrinterManager::deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds)
{
    if(!mNetworkWAN) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, false);
    }
    return mNetworkWAN->deletePrintTasks(printerId, taskIds);
}

PrinterNetworkResult<NetworkUserInfo> PrinterManager::getRtcToken()
{
    if(!mNetworkWAN) {
        return PrinterNetworkResult<NetworkUserInfo>(PrinterNetworkErrorCode::SUCCESS, NetworkUserInfo());
    }
    return mNetworkWAN->getRtcToken();
}
PrinterNetworkResult<bool> PrinterManager::sendRtmMessage(const std::string& printerId, const std::string& message)
{
    if(!mNetworkWAN) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, false);
    }
    
    auto result = mNetworkWAN->sendRtmMessage(printerId, message);
    return result;
}



} // namespace Slic3r
