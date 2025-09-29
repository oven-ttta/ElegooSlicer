#include "ElegooLink.hpp"
#include "PrinterNetworkEvent.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include "elegoolink/ElegooLink.h"
#include "libslic3r/Utils.hpp"
#include "ElegooLinkWAN.hpp"


#define ELEGOO_NETWORK_LIBRARY "ElegooNetwork"
namespace Slic3r {


namespace {

PrinterNetworkErrorCode parseElegooResult(elink::ELINK_ERROR_CODE code)
{
    switch (code) {
    case elink::ELINK_ERROR_CODE::SUCCESS: return PrinterNetworkErrorCode::SUCCESS;
    case elink::ELINK_ERROR_CODE::UNKNOWN_ERROR: return PrinterNetworkErrorCode::UNKNOWN_ERROR;
    case elink::ELINK_ERROR_CODE::NOT_INITIALIZED: return PrinterNetworkErrorCode::NOT_INITIALIZED;
    case elink::ELINK_ERROR_CODE::INVALID_PARAMETER: return PrinterNetworkErrorCode::INVALID_PARAMETER;
    case elink::ELINK_ERROR_CODE::OPERATION_TIMEOUT: return PrinterNetworkErrorCode::OPERATION_TIMEOUT;
    case elink::ELINK_ERROR_CODE::OPERATION_CANCELLED: return PrinterNetworkErrorCode::OPERATION_CANCELLED;
    case elink::ELINK_ERROR_CODE::OPERATION_IN_PROGRESS: return PrinterNetworkErrorCode::OPERATION_IN_PROGRESS;
    case elink::ELINK_ERROR_CODE::OPERATION_NOT_IMPLEMENTED: return PrinterNetworkErrorCode::OPERATION_NOT_IMPLEMENTED;
    case elink::ELINK_ERROR_CODE::NETWORK_ERROR: return PrinterNetworkErrorCode::NETWORK_ERROR;
    case elink::ELINK_ERROR_CODE::INVALID_USERNAME_OR_PASSWORD: return PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD;
    case elink::ELINK_ERROR_CODE::INVALID_TOKEN: return PrinterNetworkErrorCode::INVALID_TOKEN;
    case elink::ELINK_ERROR_CODE::INVALID_ACCESS_CODE: return PrinterNetworkErrorCode::INVALID_ACCESS_CODE;
    case elink::ELINK_ERROR_CODE::INVALID_PIN_CODE: return PrinterNetworkErrorCode::INVALID_PIN_CODE;
    case elink::ELINK_ERROR_CODE::FILE_TRANSFER_FAILED: return PrinterNetworkErrorCode::FILE_TRANSFER_FAILED;
    case elink::ELINK_ERROR_CODE::FILE_NOT_FOUND: return PrinterNetworkErrorCode::FILE_NOT_FOUND;
    case elink::ELINK_ERROR_CODE::FILE_ALREADY_EXISTS: return PrinterNetworkErrorCode::FILE_ALREADY_EXISTS;
    case elink::ELINK_ERROR_CODE::FILE_ACCESS_DENIED: return PrinterNetworkErrorCode::FILE_ACCESS_DENIED;
    case elink::ELINK_ERROR_CODE::PRINTER_NOT_FOUND: return PrinterNetworkErrorCode::PRINTER_NOT_FOUND;
    case elink::ELINK_ERROR_CODE::PRINTER_CONNECTION_ERROR: return PrinterNetworkErrorCode::PRINTER_CONNECTION_ERROR;
    case elink::ELINK_ERROR_CODE::PRINTER_CONNECTION_LIMIT_EXCEEDED: return PrinterNetworkErrorCode::PRINTER_CONNECTION_LIMIT_EXCEEDED;
    case elink::ELINK_ERROR_CODE::PRINTER_ALREADY_CONNECTED: return PrinterNetworkErrorCode::PRINTER_ALREADY_CONNECTED;
    case elink::ELINK_ERROR_CODE::PRINTER_BUSY: return PrinterNetworkErrorCode::PRINTER_BUSY;
    case elink::ELINK_ERROR_CODE::PRINTER_COMMAND_FAILED: return PrinterNetworkErrorCode::PRINTER_COMMAND_FAILED;
    case elink::ELINK_ERROR_CODE::PRINTER_UNKNOWN_ERROR: return PrinterNetworkErrorCode::PRINTER_UNKNOWN_ERROR;
    case elink::ELINK_ERROR_CODE::PRINTER_INVALID_PARAMETER: return PrinterNetworkErrorCode::PRINTER_INVALID_PARAMETER;
    case elink::ELINK_ERROR_CODE::PRINTER_INVALID_RESPONSE: return PrinterNetworkErrorCode::PRINTER_INVALID_RESPONSE;
    case elink::ELINK_ERROR_CODE::PRINTER_ACCESS_DENIED: return PrinterNetworkErrorCode::PRINTER_ACCESS_DENIED;
    case elink::ELINK_ERROR_CODE::PRINTER_PRINT_FILE_NOT_FOUND: return PrinterNetworkErrorCode::PRINTER_PRINT_FILE_NOT_FOUND;   
    case elink::ELINK_ERROR_CODE::PRINTER_MISSING_BED_LEVELING_DATA: return PrinterNetworkErrorCode::PRINTER_MISSING_BED_LEVELING_DATA;
    default: return PrinterNetworkErrorCode::UNKNOWN_ERROR;
    }
}

PrinterStatus parseElegooStatus(elink::PrinterState mainStatus, elink::PrinterSubState subStatus)
{
    PrinterStatus printerStatus = PRINTER_STATUS_UNKNOWN;
    switch (mainStatus) {
    case elink::PrinterState::OFFLINE: printerStatus = PRINTER_STATUS_OFFLINE; break;
    case elink::PrinterState::IDLE: printerStatus = PRINTER_STATUS_IDLE; break;
    case elink::PrinterState::PRINTING: printerStatus = PRINTER_STATUS_PRINTING; break;
    case elink::PrinterState::SELF_CHECKING: printerStatus = PRINTER_STATUS_SELF_CHECKING; break;
    case elink::PrinterState::AUTO_LEVELING: printerStatus = PRINTER_STATUS_AUTO_LEVELING; break;
    case elink::PrinterState::PID_CALIBRATING: printerStatus = PRINTER_STATUS_PID_CALIBRATING; break;
    case elink::PrinterState::RESONANCE_TESTING: printerStatus = PRINTER_STATUS_RESONANCE_TESTING; break;
    case elink::PrinterState::UPDATING: printerStatus = PRINTER_STATUS_UPDATING; break;
    case elink::PrinterState::FILE_COPYING: printerStatus = PRINTER_STATUS_FILE_COPYING; break;
    case elink::PrinterState::FILE_TRANSFERRING: printerStatus = PRINTER_STATUS_FILE_TRANSFERRING; break;
    case elink::PrinterState::HOMING: printerStatus = PRINTER_STATUS_HOMING; break;
    case elink::PrinterState::PREHEATING: printerStatus = PRINTER_STATUS_PREHEATING; break;
    case elink::PrinterState::FILAMENT_OPERATING: printerStatus = PRINTER_STATUS_FILAMENT_OPERATING; break;
    case elink::PrinterState::EXTRUDER_OPERATING: printerStatus = PRINTER_STATUS_EXTRUDER_OPERATING; break;
    case elink::PrinterState::UNKNOWN: printerStatus = PRINTER_STATUS_UNKNOWN; break;
    case elink::PrinterState::BUSY: printerStatus = PRINTER_STATUS_BUSY; break;
    case elink::PrinterState::EXCEPTION: printerStatus = PRINTER_STATUS_ERROR; break;
    default: printerStatus = PRINTER_STATUS_UNKNOWN; break;
    }


    switch (subStatus) {
    case elink::PrinterSubState::P_PAUSING: printerStatus = PRINTER_STATUS_PAUSING; break;
    case elink::PrinterSubState::P_PAUSED: printerStatus = PRINTER_STATUS_PAUSED; break;
    case elink::PrinterSubState::P_PRINTING_COMPLETED: printerStatus = PRINTER_STATUS_PRINT_COMPLETED; break;
    default: break;
    }
    return printerStatus;
}

PrinterNetworkInfo convertFromElegooPrinterInfo(const elink::PrinterInfo& printerInfo)
{
    PrinterNetworkInfo info;
    info.printerId = printerInfo.printerId;
    info.firmwareVersion = printerInfo.firmwareVersion;
    info.mainboardId = printerInfo.mainboardId;
    info.serialNumber = printerInfo.serialNumber;
    info.webUrl = printerInfo.webUrl;
    info.printerName = printerInfo.name;
    info.host = printerInfo.host;
    info.printerModel = printerInfo.model;
    info.vendor = printerInfo.brand;
    // std map 转json
    nlohmann::json json;
    for(const auto& [key, value] : printerInfo.extraInfo) {
        json[key] = value;
    }
    info.extraInfo = json.dump();

    info.authMode = PRINTER_AUTH_MODE_NONE;
    if(printerInfo.authMode == "basic") {
        info.authMode = PRINTER_AUTH_MODE_PASSWORD;
    } else if(printerInfo.authMode == "accessCode") {
        info.authMode = PRINTER_AUTH_MODE_ACCESS_CODE;
    } else if(printerInfo.authMode == "pinCode") {
        info.authMode = PRINTER_AUTH_MODE_PIN_CODE;
    }
    
    return info;
}
PrinterNetworkInfo convertFromElegooPrinterAttributes(const elink::PrinterAttributes& attributes)
{
    PrinterNetworkInfo info = convertFromElegooPrinterInfo(attributes);

    info.printCapabilities.supportsAutoBedLeveling = attributes.capabilities.printCapabilities.supportsAutoBedLeveling;
    info.printCapabilities.supportsTimeLapse = attributes.capabilities.printCapabilities.supportsTimeLapse;
    info.printCapabilities.supportsHeatedBedSwitching = attributes.capabilities.printCapabilities.supportsHeatedBedSwitching;
    info.printCapabilities.supportsFilamentMapping = attributes.capabilities.printCapabilities.supportsFilamentMapping;
    info.systemCapabilities.supportsMultiFilament = attributes.capabilities.systemCapabilities.supportsMultiFilament;
    info.systemCapabilities.canGetDiskInfo = attributes.capabilities.systemCapabilities.canGetDiskInfo;
    info.systemCapabilities.canSetPrinterName = attributes.capabilities.systemCapabilities.canSetPrinterName;
    
    return info;
}
elink::PrinterType getPrinterType(const PrinterNetworkInfo& printerNetworkInfo)
{
    std::string model  = printerNetworkInfo.printerModel;
    std::string vendor = printerNetworkInfo.vendor;
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
    if (model.find(vendor) == std::string::npos) {
        model = vendor + " " + model;
    }
    if (model == "elegoo centauri carbon 2") {
        return elink::PrinterType::ELEGOO_FDM_CC2;
    } else if (model == "elegoo centauri carbon" || model == "elegoo centauri") {
        return elink::PrinterType::ELEGOO_FDM_CC;
    } else if (vendor == "elegoo") {
        return elink::PrinterType::ELEGOO_FDM_KLIPPER;
    }

    return elink::PrinterType::GENERIC_FDM_KLIPPER;
}

} // namespace

ElegooLink::ElegooLink()
{
   
}

ElegooLink::~ElegooLink()
{
    
}

void ElegooLink::init()
{
    elink::ElegooLink::Config cfg;

    cfg.logLevel         = 2; // Log level 0 - TRACE, 1 - DEBUG, 2 - INFO, 3 - WARN, 4 - ERROR, 5 - CRITICAL, 6 - OFF
    cfg.logEnableConsole = true;
    cfg.logEnableFile    = true;
    cfg.logFileName      = data_dir() + "/log/elegoolink.log";
    cfg.logMaxFileSize   = 10 * 1024 * 1024;
    cfg.logMaxFiles      = 5;
    cfg.enableWebServer =true;
    cfg.webServerPort = 32538;
    auto webDir = resources_dir();
    std::replace(webDir.begin(), webDir.end(), '\\', '/');
    cfg.staticWebPath = webDir + "/web/elegoo-fdm-web";

    if (!elink::ElegooLink::getInstance().initialize(cfg)) {
        wxLogError("Error initializing ElegooLink");
    }
    std::string version = elink::ElegooLink::getInstance().getVersion();
    wxLogMessage("ElegooLink version: %s", version.c_str());
    // Subscribe to ElegooLink events
    elink::ElegooLink::getInstance().subscribeEvent<elink::PrinterConnectionEvent>(
        [&](const std::shared_ptr<elink::PrinterConnectionEvent>& event) {
            PrinterConnectStatus status = (event->connectionStatus.status == elink::ConnectionStatus::CONNECTED) ?
                                              PRINTER_CONNECT_STATUS_CONNECTED :
                                              PRINTER_CONNECT_STATUS_DISCONNECTED;

            PrinterNetworkEvent::getInstance()->connectStatusChanged.emit(
                PrinterConnectStatusEvent(event->connectionStatus.printerId, status));
        });

    elink::ElegooLink::getInstance().subscribeEvent<elink::PrinterStatusEvent>([&](const std::shared_ptr<elink::PrinterStatusEvent>& event) {
        
        PrinterStatus status = parseElegooStatus(event->status.printerStatus.state, event->status.printerStatus.subState);
        PrinterNetworkEvent::getInstance()->statusChanged.emit(PrinterStatusEvent(event->status.printerId, status));


        PrinterPrintTask task;
        task.taskId        = event->status.printStatus.taskId;
        task.fileName      = event->status.printStatus.fileName;
        task.totalTime     = event->status.printStatus.totalTime;
        task.currentTime   = event->status.printStatus.currentTime;
        task.estimatedTime = event->status.printStatus.estimatedTime;
        task.progress      = event->status.printStatus.progress;

        PrinterNetworkEvent::getInstance()->printTaskChanged.emit(PrinterPrintTaskEvent(event->status.printerId, task));
    });

    elink::ElegooLink::getInstance().subscribeEvent<elink::PrinterAttributesEvent>([&](const std::shared_ptr<elink::PrinterAttributesEvent>& event) {
        PrinterNetworkInfo info = convertFromElegooPrinterAttributes(event->attributes);
        PrinterNetworkEvent::getInstance()->attributesChanged.emit(PrinterAttributesEvent(event->attributes.printerId, info));
    });
 
}

void ElegooLink::uninit()
{
    elink::ElegooLink::getInstance().cleanup();    
}

std::string parseUnknownErrorMsg(PrinterNetworkErrorCode resultCode, const std::string& msg)
{
    if(resultCode == PrinterNetworkErrorCode::PRINTER_UNKNOWN_ERROR) {
        if(msg.find("[ErrorCode:") != std::string::npos) {
            return getErrorMessage(resultCode) + "(" + msg.substr(msg.find("[ErrorCode:") + 11, msg.find("]") - msg.find("[ErrorCode:") - 11) + ")";
        }
    }
    return "";
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooLink::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    elink::ConnectPrinterResult elinkResult;
    try {
        elink::ConnectPrinterParams connectionParams;
        connectionParams.printerId        = printerNetworkInfo.printerId;
        connectionParams.printerType = getPrinterType(printerNetworkInfo);
        connectionParams.name            = printerNetworkInfo.printerName;
        if(printerNetworkInfo.authMode == PRINTER_AUTH_MODE_PASSWORD) {
            connectionParams.authMode        = "basic";
        } else if(printerNetworkInfo.authMode == PRINTER_AUTH_MODE_ACCESS_CODE) {
            connectionParams.authMode        = "accessCode";
        } else if(printerNetworkInfo.authMode == PRINTER_AUTH_MODE_PIN_CODE) {
            connectionParams.authMode        = "pinCode";
        } else {
            connectionParams.authMode        = "";
        }
        connectionParams.username        = printerNetworkInfo.username;
        connectionParams.password        = printerNetworkInfo.password;
        connectionParams.host            = printerNetworkInfo.host;
        connectionParams.model           = printerNetworkInfo.printerModel;
        connectionParams.serialNumber    = printerNetworkInfo.serialNumber;
        connectionParams.brand           = printerNetworkInfo.vendor;
        connectionParams.autoReconnect   = true;
        connectionParams.checkConnection = true; 
        connectionParams.connectionTimeout = 3 * 1000;
        connectionParams.token             = printerNetworkInfo.token;
        connectionParams.accessCode        = printerNetworkInfo.accessCode;
        connectionParams.pinCode           = printerNetworkInfo.pinCode;

        elinkResult = elink::ElegooLink::getInstance().connectPrinter(connectionParams);
        resultCode    = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ELINK_ERROR_CODE::SUCCESS) {
            if (elinkResult.data.has_value() && elinkResult.data.value().isConnected && elinkResult.data.value().printerInfo.printerId == printerNetworkInfo.printerId) {      
                auto addPrinter = elinkResult.data.value().printerInfo;
                PrinterNetworkInfo info = convertFromElegooPrinterInfo(addPrinter);
                return PrinterNetworkResult<PrinterNetworkInfo>(resultCode, info, parseUnknownErrorMsg(resultCode, elinkResult.message));
            } else {
                resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_INVALID_DATA;
            }

        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::connectToPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<PrinterNetworkInfo>(resultCode, printerNetworkInfo, parseUnknownErrorMsg(resultCode, elinkResult.message));
}


PrinterNetworkResult<bool> ElegooLink::disconnectFromPrinter(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    elink::VoidResult elinkResult;
    try {
        elinkResult = elink::ElegooLink::getInstance().disconnectPrinter(printerId);
        resultCode    = parseElegooResult(elinkResult.code);

    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::disconnectFromPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS, parseUnknownErrorMsg(resultCode, elinkResult.message));
}


PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooLink::discoverPrinters()
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    std::vector<PrinterNetworkInfo> discoverPrinters;
    elink::PrinterDiscoveryResult elinkResult;
    try {
        elink::PrinterDiscoveryParams discoveryParams;
        discoveryParams.timeoutMs         = 5 * 1000;
        discoveryParams.broadcastInterval = 1000;
        discoveryParams.enableAutoRetry   = true;
        elinkResult = elink::ElegooLink::getInstance().startPrinterDiscovery(discoveryParams);
        resultCode  = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ELINK_ERROR_CODE::SUCCESS && elinkResult.data.has_value()) {
            for (const auto& printer : elinkResult.value().printers) {
                PrinterNetworkInfo info = convertFromElegooPrinterInfo(printer);
                discoverPrinters.push_back(info);
            }
        } 

        // WAN
        if(elink::ElegooLinkWAN::getInstance()->isInitialized()) {
            auto elinkResultWAN = elink::ElegooLinkWAN::getInstance()->getPrinters();
            resultCode = parseElegooResult(elinkResultWAN.code);
            if(elinkResultWAN.code == elink::ELINK_ERROR_CODE::SUCCESS && elinkResultWAN.data.has_value()) {
                for(const auto& printer : elinkResultWAN.value().printers) {
                    PrinterNetworkInfo info = convertFromElegooPrinterInfo(printer);
                    info.isWAN = true;
                    discoverPrinters.push_back(info);
                }
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::discoverPrinters: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(resultCode, discoverPrinters, parseUnknownErrorMsg(resultCode, elinkResult.message));
}

bool ElegooLink::isBusy(const std::string& printerId, PrinterStatus &status, int tryCount)
{
    if(tryCount < 1){
        tryCount = 1;
    }
    bool isBusy = true;
    status = PRINTER_STATUS_UNKNOWN;
    for(int i = 0; i < tryCount; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elinkResult = elink::ElegooLink::getInstance().getPrinterStatus({printerId});
        if (elinkResult.code == elink::ELINK_ERROR_CODE::SUCCESS) {
            const auto& statusData = elinkResult.value();
            if(statusData.printerStatus.state == elink::PrinterState::IDLE) {
                isBusy = false;
                status = PRINTER_STATUS_IDLE;
                break;
            } else {
                status = parseElegooStatus(statusData.printerStatus.state, statusData.printerStatus.subState);
            }
        } else {
            return false;
        }
    }
    return isBusy;
}

PrinterNetworkResult<bool> ElegooLink::sendPrintTask(const PrinterNetworkParams& params)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    elink::StartPrintResult elinkResult;
    try {
        PrinterStatus status;
        if(isBusy(params.printerId, status)) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_BUSY, false, "Printer is busy, status: " + std::to_string(status));
        }
        elink::StartPrintParams startPrintParams;
        startPrintParams.printerId        = params.printerId;
        startPrintParams.storageLocation = "local";
        startPrintParams.fileName         = params.fileName;
        startPrintParams.autoBedLeveling = params.heatedBedLeveling;
        startPrintParams.heatedBedType   = params.bedType;
        startPrintParams.enableTimeLapse = params.timeLapse;
        startPrintParams.slotMap.clear();
        
        for(const auto& filamentMmsMapping : params.filamentMmsMappingList) {
            if (filamentMmsMapping.mappedMmsFilament.mmsId.empty() || filamentMmsMapping.mappedMmsFilament.trayId.empty()) {
                continue;
            }
            elink::SlotMapItem slotMapItem;
            slotMapItem.t = filamentMmsMapping.index;
            slotMapItem.canvasId = std::stoi(filamentMmsMapping.mappedMmsFilament.mmsId);
            slotMapItem.trayId = std::stoi(filamentMmsMapping.mappedMmsFilament.trayId);
            startPrintParams.slotMap.push_back(slotMapItem);
        }

        elink::VoidResult autoRefillResult;
        if(params.hasMms) {
        if(params.autoRefill) {
            autoRefillResult = elink::ElegooLink::getInstance().setAutoRefill({params.printerId, true});
            } else {
                autoRefillResult = elink::ElegooLink::getInstance().setAutoRefill({params.printerId, false});
            }
        }
        
        resultCode = parseElegooResult(autoRefillResult.code);
        if(resultCode != PrinterNetworkErrorCode::SUCCESS) {
            return PrinterNetworkResult<bool>(resultCode, false, parseUnknownErrorMsg(resultCode, autoRefillResult.message));
        }

        elinkResult = elink::ElegooLink::getInstance().startPrint(startPrintParams);
        resultCode  = parseElegooResult(elinkResult.code);
        if(resultCode == PrinterNetworkErrorCode::PRINTER_MISSING_BED_LEVELING_DATA && startPrintParams.autoBedLeveling == false) {
            //missing bed leveling data, send bed leveling data
            startPrintParams.autoBedLeveling = true;
            startPrintParams.bedLevelForce = true;
            elinkResult = elink::ElegooLink::getInstance().startPrint(startPrintParams);
            resultCode = parseElegooResult(elinkResult.code);
        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::sendPrintTask: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS, parseUnknownErrorMsg(resultCode, elinkResult.message));
}


PrinterNetworkResult<bool> ElegooLink::sendPrintFile(const PrinterNetworkParams& params)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    elink::FileUploadCompletionResult elinkResult;
    try {
        PrinterStatus status;
        if(isBusy(params.printerId, status , 1)) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_BUSY, false);
        }
        elink::FileUploadParams uploadParams;
        uploadParams.printerId = params.printerId;
        uploadParams.storageLocation = "local";
        uploadParams.localFilePath   = params.filePath;
        uploadParams.fileName = params.fileName;

        elinkResult = elink::ElegooLink::getInstance().uploadFile(uploadParams, [&](const elink::FileUploadProgressData& progress) -> bool {
            if(params.uploadProgressFn) {
                bool cancel = false;
                params.uploadProgressFn(progress.uploadedBytes, progress.totalBytes, cancel);
                if(cancel) {
                    return false;
                }
            }   
            return true; 
        });
        resultCode = parseElegooResult(elinkResult.code);
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::sendPrintFile: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    PrinterNetworkResult<bool> networkResult(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS, parseUnknownErrorMsg(resultCode, elinkResult.message)); 
    return networkResult;
}


PrinterNetworkResult<PrinterMmsGroup> ElegooLink::getPrinterMmsInfo(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    elink::GetCanvasStatusResult elinkResult;
    PrinterMmsGroup mmsGroup;
    mmsGroup.mmsSystemName = "CANVAS";
    try {
        elinkResult = elink::ElegooLink::getInstance().getCanvasStatus({printerId});
        resultCode = parseElegooResult(elinkResult.code);
        if(resultCode == PrinterNetworkErrorCode::SUCCESS) {
            if(elinkResult.hasData()) {
                const auto& mmsData = elinkResult.value();
                mmsGroup.connectNum = 0;
                mmsGroup.connected = false;
                mmsGroup.activeMmsId = std::to_string(mmsData.activeCanvasId);
                mmsGroup.activeTrayId = std::to_string(mmsData.activeTrayId);
                mmsGroup.autoRefill = mmsData.autoRefill;
                for(const auto& canvas : mmsData.canvases) {
                    PrinterMms mmsInfo;
                    mmsInfo.mmsId = std::to_string(canvas.canvasId);
                    mmsInfo.connected = canvas.connected;
                    if(canvas.connected) {
                        mmsGroup.connected = true;
                        mmsGroup.connectNum++;
                    }
                    mmsInfo.mmsName = canvas.name;
                    mmsInfo.mmsModel = canvas.model;
                    for(const auto& tray : canvas.trays) {
                        PrinterMmsTray trayInfo;
                        trayInfo.trayName = "";
                        trayInfo.trayId = std::to_string(tray.trayId);
                        trayInfo.mmsId = std::to_string(canvas.canvasId);
                        trayInfo.vendor = tray.brand;
                        trayInfo.filamentType = tray.filamentType;
                        trayInfo.filamentName = tray.filamentName;
                        trayInfo.filamentColor = tray.filamentColor;
                        trayInfo.minNozzleTemp = tray.minNozzleTemp;
                        trayInfo.maxNozzleTemp = tray.maxNozzleTemp;
                        switch (tray.status) {
                        case 0:
                            trayInfo.status = TRAY_STATUS_DISCONNECTED;
                            break;
                        case 1:
                            if(tray.filamentType.empty() || tray.filamentName.empty()) {
                                trayInfo.status = TRAY_STATUS_PRELOADED_UNKNOWN_FILAMENT;
                            } else {
                                trayInfo.status = TRAY_STATUS_PRELOADED;
                            }
                            break;
                        case 2:
                            if(tray.filamentType.empty() || tray.filamentName.empty()) {
                                trayInfo.status = TRAY_STATUS_LOADED_UNKNOWN_FILAMENT;
                            } else {
                                trayInfo.status = TRAY_STATUS_LOADED;
                            }
                            break;
                        default:
                            trayInfo.status = TRAY_STATUS_ERROR;
                            break;
                        }
                        mmsInfo.trayList.push_back(trayInfo);
                    }
                    mmsGroup.mmsList.push_back(mmsInfo);                 
                }
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::getPrinterMmsInfo: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<PrinterMmsGroup>(resultCode, mmsGroup, parseUnknownErrorMsg(resultCode, elinkResult.message));
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooLink::getPrinterAttributes(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    elink::PrinterAttributesResult elinkResult;
    PrinterNetworkInfo printerNetworkInfo;
    try {
        elinkResult = elink::ElegooLink::getInstance().getPrinterAttributes({printerId});
        resultCode = parseElegooResult(elinkResult.code);
        if(resultCode == PrinterNetworkErrorCode::SUCCESS) {
            if(elinkResult.hasData()) {
                const elink::PrinterAttributes& attributes = elinkResult.value();
                printerNetworkInfo = convertFromElegooPrinterAttributes(attributes);
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::getPrinterAttributes: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<PrinterNetworkInfo>(resultCode, printerNetworkInfo, parseUnknownErrorMsg(resultCode, elinkResult.message));
}


PrinterNetworkResult<std::vector<PrinterPrintFile>> ElegooLink::getFileList(const std::string& printerId)
{
    std::vector<PrinterPrintFile> printFiles;
   // return elink::ElegooLinkWAN::getInstance()->getFileList(elink::GetFileListParams{printerId});
    return PrinterNetworkResult<std::vector<PrinterPrintFile>>(PrinterNetworkErrorCode::SUCCESS, printFiles);
}

PrinterNetworkResult<std::vector<PrinterPrintTask>> ElegooLink::getPrintTaskList(const std::string& printerId)
{
    std::vector<PrinterPrintTask> printTasks;
    //return elink::ElegooLinkWAN::getInstance()->getPrintTaskList(elink::GetPrintTaskListParams{printerId});
    return PrinterNetworkResult<std::vector<PrinterPrintTask>>(PrinterNetworkErrorCode::SUCCESS, printTasks);
}

PrinterNetworkResult<bool> ElegooLink::deletePrintTasks(const std::string& printerId, const std::vector<std::string>& taskIds)
{
    //return elink::ElegooLinkWAN::getInstance()->deletePrintTasks(elink::DeletePrintTasksParams{printerId, taskIds});
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}
PrinterNetworkResult<std::string> ElegooLink::hasInstalledPlugin()
{
    std::string version = "";
    if(elink::ElegooLinkWAN::getInstance()->isInitialized()) {
        version = elink::ElegooLinkWAN::getInstance()->version();
    }
    return PrinterNetworkResult<std::string>(PrinterNetworkErrorCode::SUCCESS, version);
}

PrinterNetworkResult<bool> ElegooLink::installPlugin(const std::string& pluginPath)
{
     std::string libraryPath;
 #if defined(_MSC_VER) || defined(_WIN32)
     libraryPath = pluginPath + "\\" + std::string(ELEGOO_NETWORK_LIBRARY) + ".dll";
 #else
     #if defined(__WXMAC__)
     libraryPath = pluginPath + "/" + std::string("lib") + std::string(ELEGOO_NETWORK_LIBRARY) + ".dylib";
     #else
     libraryPath = pluginPath + "/" + std::string("lib") + std::string(ELEGOO_NETWORK_LIBRARY) + ".so";
     #endif
 #endif

    uninstallPlugin();

    elink::NetworkConfig config;
    config.logLevel         = 1;
    config.logEnableConsole = true;
    config.logEnableFile    = true;
    config.logFileName      = data_dir() + "/log/elegoonetwork.log";
    config.logMaxFileSize   = 10 * 1024 * 1024;


    auto loadResult = elink::ElegooLinkWAN::getInstance()->loadLibrary(libraryPath);
    if (loadResult != elink::LoaderResult::SUCCESS) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION, false, "Failed to load ElegooNetwork library");
    }

    elink::ElegooLinkWAN::getInstance()->initialize(config);

    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}

PrinterNetworkResult<bool> ElegooLink::uninstallPlugin()
{
    if(elink::ElegooLinkWAN::getInstance()->isInitialized()) {
        elink::ElegooLinkWAN::getInstance()->cleanup();
        elink::ElegooLinkWAN::getInstance()->unloadLibrary();
    }
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}

PrinterNetworkResult<bool> ElegooLink::loginWAN(const NetworkUserInfo& userInfo)
{
    auto result = elink::ElegooLinkWAN::getInstance()->setHttpCredential(elink::HttpCredential{userInfo.userId, userInfo.token, userInfo.refreshToken, (long long)userInfo.accessTokenExpireTime, (long long)userInfo.refreshTokenExpireTime});
    if(result.code != elink::ELINK_ERROR_CODE::SUCCESS) {
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION, false, "Failed to login WAN");
    }
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}

PrinterNetworkResult<std::string> ElegooLink::getRtcToken()
{
    //return elink::ElegooLinkWAN::getInstance()->getRtcToken();
    return PrinterNetworkResult<std::string>(PrinterNetworkErrorCode::SUCCESS, "");
}

PrinterNetworkResult<bool> ElegooLink::sendRtmMessage(const std::string& message)
{
    //return elink::ElegooLinkWAN::getInstance()->sendRtmMessage(elink::SendRtmMessageParams{message});
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
}


} // namespace Slic3r