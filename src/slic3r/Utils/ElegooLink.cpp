#include "ElegooLink.hpp"
#include "PrinterNetworkEvent.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include "elegoolink/ElegooLink.h"
#include "libslic3r/Utils.hpp"
namespace Slic3r {


namespace {

PrinterNetworkErrorCode parseElegooResult(elink::ElegooError code)
{
    switch (code) {
    case elink::ElegooError::SUCCESS: return PrinterNetworkErrorCode::SUCCESS;
    case elink::ElegooError::BAD_REQUEST: return PrinterNetworkErrorCode::BAD_REQUEST;
    case elink::ElegooError::INTERNAL_ERROR: return PrinterNetworkErrorCode::INTERNAL_ERROR;
    case elink::ElegooError::INVALID_PARAMETER: return PrinterNetworkErrorCode::INVALID_PARAMETER;
    case elink::ElegooError::INVALID_FORMAT: return PrinterNetworkErrorCode::INVALID_FORMAT;
    case elink::ElegooError::TIMEOUT: return PrinterNetworkErrorCode::TIMEOUT;
    case elink::ElegooError::CANCELLED: return PrinterNetworkErrorCode::CANCELLED;
    case elink::ElegooError::ACCESS_DENIED: return PrinterNetworkErrorCode::ACCESS_DENIED;
    case elink::ElegooError::NOT_FOUND: return PrinterNetworkErrorCode::NOT_FOUND;
    case elink::ElegooError::ALREADY_EXISTS: return PrinterNetworkErrorCode::ALREADY_EXISTS;
    case elink::ElegooError::INSUFFICIENT_SPACE: return PrinterNetworkErrorCode::INSUFFICIENT_SPACE;
    case elink::ElegooError::CONNECTION_ERROR: return PrinterNetworkErrorCode::CONNECTION_ERROR;
    case elink::ElegooError::NETWORK_ERROR: return PrinterNetworkErrorCode::NETWORK_ERROR;
    case elink::ElegooError::SERVICE_UNAVAILABLE: return PrinterNetworkErrorCode::SERVICE_UNAVAILABLE;
    case elink::ElegooError::NOT_IMPLEMENTED: return PrinterNetworkErrorCode::NOT_IMPLEMENTED;
    case elink::ElegooError::VERSION_NOT_SUPPORTED: return PrinterNetworkErrorCode::VERSION_NOT_SUPPORTED;
    case elink::ElegooError::VERSION_TOO_OLD: return PrinterNetworkErrorCode::VERSION_TOO_OLD;
    case elink::ElegooError::VERSION_TOO_NEW: return PrinterNetworkErrorCode::VERSION_TOO_NEW;
    case elink::ElegooError::NOT_AUTHORIZED: return PrinterNetworkErrorCode::NOT_AUTHORIZED;
    case elink::ElegooError::INVALID_CREDENTIAL: return PrinterNetworkErrorCode::INVALID_CREDENTIAL;
    case elink::ElegooError::TOKEN_EXPIRED: return PrinterNetworkErrorCode::TOKEN_EXPIRED;
    case elink::ElegooError::FILE_TRANSFER_FAILED: return PrinterNetworkErrorCode::FILE_TRANSFER_FAILED;
    case elink::ElegooError::FILE_NOT_FOUND: return PrinterNetworkErrorCode::FILE_NOT_FOUND;
    case elink::ElegooError::PRINTER_BUSY: return PrinterNetworkErrorCode::PRINTER_BUSY;
    case elink::ElegooError::PRINTER_OFFLINE: return PrinterNetworkErrorCode::PRINTER_OFFLINE;
    case elink::ElegooError::PRINTER_INITIALIZATION_ERROR: return PrinterNetworkErrorCode::PRINTER_INITIALIZATION_ERROR;
    case elink::ElegooError::PRINTER_COMMAND_FAILED: return PrinterNetworkErrorCode::PRINTER_COMMAND_FAILED;
    case elink::ElegooError::PRINTER_ALREADY_CONNECTED: return PrinterNetworkErrorCode::PRINTER_ALREADY_CONNECTED;
    case elink::ElegooError::PRINTER_INTERNAL_ERROR: return PrinterNetworkErrorCode::PRINTER_INTERNAL_ERROR;
    case elink::ElegooError::PRINTER_TASK_NOT_FOUND: return PrinterNetworkErrorCode::PRINTER_TASK_NOT_FOUND;
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

    cfg.logLevel         = 1;
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
        PrinterNetworkInfo info;
        info.printerId       = event->attributes.printerId;
        info.firmwareVersion = event->attributes.firmwareVersion;
        PrinterNetworkEvent::getInstance()->attributesChanged.emit(PrinterAttributesEvent(event->attributes.printerId, info));
    });

}

void ElegooLink::uninit()
{
    elink::ElegooLink::getInstance().cleanup();    
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooLink::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    try {
        elink::ConnectPrinterParams connectionParams;
        connectionParams.printerId        = printerNetworkInfo.printerId;
        if(printerNetworkInfo.isPhysicalPrinter && printerNetworkInfo.printerType == -1) {
            connectionParams.printerType = (elink::PrinterType) getPrinterType(printerNetworkInfo);
        } else {
            connectionParams.printerType = (elink::PrinterType) printerNetworkInfo.printerType;
        } 
        connectionParams.name            = printerNetworkInfo.printerName;
        connectionParams.authMode        = printerNetworkInfo.authMode;
        connectionParams.username        = printerNetworkInfo.username;
        connectionParams.password        = printerNetworkInfo.password;
        connectionParams.host            = printerNetworkInfo.host;
        connectionParams.model           = printerNetworkInfo.printerModel;
        connectionParams.serialNumber    = printerNetworkInfo.serialNumber;
        connectionParams.brand           = printerNetworkInfo.vendor;
        connectionParams.autoReconnect   = true;
        connectionParams.checkConnection = true; 
        connectionParams.connectionTimeout = 3 * 1000;
        if (printerNetworkInfo.authMode == "basic") {
            connectionParams.username = "";
            connectionParams.password = "";
        } else if (printerNetworkInfo.authMode == "token") {
            nlohmann::json extraInfo = nlohmann::json::parse(printerNetworkInfo.extraInfo);
            if (extraInfo.contains(PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN)) {
                connectionParams.token = extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN].get<std::string>();
            } else {
                wxLogError("Error connecting to printer: %s", "Token is not set");
            }
        }
        auto elinkResult = elink::ElegooLink::getInstance().connectPrinter(connectionParams);
        resultCode    = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ElegooError::SUCCESS) {
            if (elinkResult.data.has_value() && elinkResult.data.value().isConnected && elinkResult.data.value().printerInfo.printerId == printerNetworkInfo.printerId) {      
                auto addPrinter = elinkResult.data.value().printerInfo;
                PrinterNetworkInfo info = printerNetworkInfo;
                //update printer network info
                info.firmwareVersion = addPrinter.firmwareVersion;
                info.webUrl          = addPrinter.webUrl;
                info.mainboardId     = addPrinter.mainboardId;
                info.serialNumber    = addPrinter.serialNumber;
                info.firmwareVersion = addPrinter.firmwareVersion;
                info.authMode = addPrinter.authMode;
                info.printerId = addPrinter.printerId;
                return PrinterNetworkResult<PrinterNetworkInfo>(resultCode, info);
            } else {
                resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_INVALID_DATA;
            }

        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::connectToPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<PrinterNetworkInfo>(resultCode, printerNetworkInfo);
}


PrinterNetworkResult<bool> ElegooLink::disconnectFromPrinter(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    try {
        auto elinkResult = elink::ElegooLink::getInstance().disconnectPrinter(printerId);
        resultCode    = parseElegooResult(elinkResult.code);

    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::disconnectFromPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS);
}


PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooLink::discoverPrinters()
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    std::vector<PrinterNetworkInfo> discoverPrinters;
    try {
        elink::PrinterDiscoveryParams discoveryParams;
        discoveryParams.timeoutMs         = 5 * 1000;
        discoveryParams.broadcastInterval = 1000;
        discoveryParams.enableAutoRetry   = true;
        auto elinkResult = elink::ElegooLink::getInstance().startPrinterDiscovery(discoveryParams);
        resultCode  = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ElegooError::SUCCESS && elinkResult.data.has_value()) {
            for (const auto& printer : elinkResult.value().printers) {
                PrinterNetworkInfo info;
                info.printerName     = printer.name;
                info.host            = printer.host;
                info.printerModel    = printer.model;
                info.firmwareVersion = printer.firmwareVersion;
                info.serialNumber    = printer.serialNumber;
                info.vendor          = printer.brand;
                info.printerType      = static_cast<int>(printer.printerType);
                info.webUrl          = printer.webUrl;

                info.authMode        = printer.authMode;
                info.mainboardId     = printer.mainboardId;
                discoverPrinters.push_back(info);
            }
        } 
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::discoverPrinters: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(resultCode, discoverPrinters);
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
        if (elinkResult.code == elink::ElegooError::SUCCESS) {
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
    try {
        PrinterStatus status;
        if(isBusy(params.printerId, status)) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_BUSY, false, "Printer is busy, status: " + std::to_string(status));
        }
        elink::StartPrintParams startPrintParams;
        startPrintParams.printerId        = params.printerId;
        startPrintParams.storageLocation = "local";
        startPrintParams.fileName        = params.fileName;
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

        auto elinkResult = elink::ElegooLink::getInstance().startPrint(startPrintParams);
        resultCode  = parseElegooResult(elinkResult.code);
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::sendPrintTask: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS);
}


PrinterNetworkResult<bool> ElegooLink::sendPrintFile(const PrinterNetworkParams& params)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    try {
        PrinterStatus status;
        if(isBusy(params.printerId, status , 1)) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_BUSY, false, "Printer is busy, status: " + std::to_string(status));
        }
        elink::FileUploadParams uploadParams;
        uploadParams.printerId = params.printerId;
        uploadParams.storageLocation = "local";
        uploadParams.localFilePath = params.filePath;
        uploadParams.fileName = params.fileName;

        auto elinkResult = elink::ElegooLink::getInstance().uploadFile(uploadParams, [&](const elink::FileUploadProgressData& progress) -> bool {
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
    PrinterNetworkResult<bool> networkResult(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS); 
    return networkResult;
}

int ElegooLink::getPrinterType(const PrinterNetworkInfo& printerNetworkInfo)
{
    if(printerNetworkInfo.isPhysicalPrinter) {
        std::string model = printerNetworkInfo.printerModel;
        std::string vendor = printerNetworkInfo.vendor;
        std::transform(model.begin(), model.end(), model.begin(), ::tolower);
        std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
        if(model.find(vendor) == std::string::npos) {
            model = vendor + " " + model;
        }
        if(model == "elegoo centauri carbon 2" || model == "elegoo centauri 2") {
            return static_cast<int>(elink::PrinterType::ELEGOO_FDM_CC2);
        } else if(model == "elegoo centauri carbon" || model == "elegoo centauri") {
            return static_cast<int>(elink::PrinterType::ELEGOO_FDM_CC);  
        } else if (vendor == "elegoo") {
            return static_cast<int>(elink::PrinterType::ELEGOO_FDM_KLIPPER);
        } else {
            return static_cast<int>(elink::PrinterType::GENERIC_FDM_KLIPPER);
        }
    } 
    return printerNetworkInfo.printerType; 
}

PrinterNetworkResult<PrinterMmsGroup> ElegooLink::getPrinterMmsInfo(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    PrinterMmsGroup mmsGroup;
    try {
        auto elinkResult = elink::ElegooLink::getInstance().getCanvasStatus({printerId});
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
    return PrinterNetworkResult<PrinterMmsGroup>(resultCode, mmsGroup);
}

PrinterNetworkResult<PrinterAttributes> ElegooLink::getPrinterAttributes(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    PrinterAttributes printerAttributes;
    try {
        elink::PrinterAttributesResult elinkResult = elink::ElegooLink::getInstance().getPrinterAttributes({printerId});
        resultCode = parseElegooResult(elinkResult.code);
        if(resultCode == PrinterNetworkErrorCode::SUCCESS) {
            if(elinkResult.hasData()) {
                const elink::PrinterAttributes& attributes = elinkResult.value();
                printerAttributes.capabilities.supportsAutoBedLeveling = attributes.capabilities.printCapabilities.supportsAutoBedLeveling;
                printerAttributes.capabilities.supportsTimeLapse = attributes.capabilities.printCapabilities.supportsTimeLapse;
                printerAttributes.capabilities.supportsHeatedBedSwitching = attributes.capabilities.printCapabilities.supportsHeatedBedSwitching;
                printerAttributes.capabilities.supportsMms = attributes.capabilities.printCapabilities.supportsFilamentMapping;
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::getPrinterAttributes: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<PrinterAttributes>(resultCode, printerAttributes);
}
} // namespace Slic3r