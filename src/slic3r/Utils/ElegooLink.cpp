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
    case elink::ElegooError::PERMISSION_DENIED: return PrinterNetworkErrorCode::PERMISSION_DENIED;
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
    case elink::ElegooError::UNAUTHORIZED: return PrinterNetworkErrorCode::UNAUTHORIZED;
    case elink::ElegooError::AUTHENTICATION_FAILED: return PrinterNetworkErrorCode::AUTHENTICATION_FAILED;
    case elink::ElegooError::TOKEN_EXPIRED: return PrinterNetworkErrorCode::TOKEN_EXPIRED;
    case elink::ElegooError::TOKEN_INVALID: return PrinterNetworkErrorCode::TOKEN_INVALID;
    case elink::ElegooError::FILE_TRANSFER_FAILED: return PrinterNetworkErrorCode::FILE_TRANSFER_FAILED;
    case elink::ElegooError::FILE_NOT_FOUND: return PrinterNetworkErrorCode::FILE_NOT_FOUND;
    case elink::ElegooError::DEVICE_BUSY: return PrinterNetworkErrorCode::PRINTER_BUSY;
    case elink::ElegooError::DEVICE_OFFLINE: return PrinterNetworkErrorCode::PRINTER_OFFLINE;
    case elink::ElegooError::DEVICE_INITIALIZATION_ERROR: return PrinterNetworkErrorCode::PRINTER_INITIALIZATION_ERROR;
    case elink::ElegooError::DEVICE_COMMAND_FAILED: return PrinterNetworkErrorCode::PRINTER_COMMAND_FAILED;
    case elink::ElegooError::DEVICE_ALREADY_CONNECTED: return PrinterNetworkErrorCode::PRINTER_ALREADY_CONNECTED;
    case elink::ElegooError::DEVICE_INTERNAL_ERROR: return PrinterNetworkErrorCode::PRINTER_INTERNAL_ERROR;
    case elink::ElegooError::DEVICE_TASK_NOT_FOUND: return PrinterNetworkErrorCode::PRINTER_TASK_NOT_FOUND;
    default: return PrinterNetworkErrorCode::UNKNOWN_ERROR;
    }
}

PrinterStatus parseElegooStatus(elink::MachineMainStatus mainStatus, elink::MachineSubStatus subStatus)
{
    PrinterStatus printerStatus = PRINTER_STATUS_UNKNOWN;
    switch (mainStatus) {
    case elink::MachineMainStatus::OFFLINE: printerStatus = PRINTER_STATUS_OFFLINE; break;
    case elink::MachineMainStatus::IDLE: printerStatus = PRINTER_STATUS_IDLE; break;
    case elink::MachineMainStatus::PRINTING: printerStatus = PRINTER_STATUS_PRINTING; break;
    case elink::MachineMainStatus::SELF_CHECKING: printerStatus = PRINTER_STATUS_SELF_CHECKING; break;
    case elink::MachineMainStatus::AUTO_LEVELING: printerStatus = PRINTER_STATUS_AUTO_LEVELING; break;
    case elink::MachineMainStatus::PID_CALIBRATING: printerStatus = PRINTER_STATUS_PID_CALIBRATING; break;
    case elink::MachineMainStatus::RESONANCE_TESTING: printerStatus = PRINTER_STATUS_RESONANCE_TESTING; break;
    case elink::MachineMainStatus::UPDATING: printerStatus = PRINTER_STATUS_UPDATING; break;
    case elink::MachineMainStatus::FILE_COPYING: printerStatus = PRINTER_STATUS_FILE_COPYING; break;
    case elink::MachineMainStatus::FILE_TRANSFERRING: printerStatus = PRINTER_STATUS_FILE_TRANSFERRING; break;
    case elink::MachineMainStatus::HOMING: printerStatus = PRINTER_STATUS_HOMING; break;
    case elink::MachineMainStatus::PREHEATING: printerStatus = PRINTER_STATUS_PREHEATING; break;
    case elink::MachineMainStatus::FILAMENT_OPERATING: printerStatus = PRINTER_STATUS_FILAMENT_OPERATING; break;
    case elink::MachineMainStatus::EXTRUDER_OPERATING: printerStatus = PRINTER_STATUS_EXTRUDER_OPERATING; break;
    case elink::MachineMainStatus::UNKNOWN: printerStatus = PRINTER_STATUS_UNKNOWN; break;
    case elink::MachineMainStatus::EXCEPTION: printerStatus = PRINTER_STATUS_ERROR; break;
    default: printerStatus = PRINTER_STATUS_UNKNOWN; break;
    }


    switch (subStatus) {
    case elink::MachineSubStatus::P_PAUSING: printerStatus = PRINTER_STATUS_PAUSING; break;
    case elink::MachineSubStatus::P_PAUSED: printerStatus = PRINTER_STATUS_PAUSED; break;
    default: break;
    }
    return printerStatus;
}

elink::ConnectDeviceParams setupConnectionParams(const PrinterNetworkInfo& printerNetworkInfo, bool checkConnection = false)
{
    elink::ConnectDeviceParams connectionParams;
    connectionParams.deviceId        = printerNetworkInfo.printerId;
    if(printerNetworkInfo.isPhysicalPrinter && printerNetworkInfo.deviceType == -1) {
        connectionParams.deviceType = (elink::DeviceType) ElegooLink::getDeviceType(printerNetworkInfo);
    } else {
        connectionParams.deviceType = (elink::DeviceType) printerNetworkInfo.deviceType;
    } 
    connectionParams.name            = printerNetworkInfo.printerName;
    connectionParams.authMode        = printerNetworkInfo.authMode;
    connectionParams.username        = printerNetworkInfo.username;
    connectionParams.password        = printerNetworkInfo.password;
    connectionParams.host            = printerNetworkInfo.host;
    connectionParams.model           = printerNetworkInfo.printerModel;
    connectionParams.serialNumber    = printerNetworkInfo.serialNumber;
    connectionParams.brand           = printerNetworkInfo.vendor;
    connectionParams.checkConnection = checkConnection;

    if (printerNetworkInfo.authMode == "basic") {
        connectionParams.username = "";
        connectionParams.password = "";
    } else if (printerNetworkInfo.authMode == "token") {
        nlohmann::json extraInfo = nlohmann::json::parse(printerNetworkInfo.extraInfo);
        if (extraInfo.contains(PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN)) {
            connectionParams.token = extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN].get<std::string>();
        } else {
            wxLogError("Error connecting to device: %s", "Token is not set");
        }
    }

    return connectionParams;
}
} // namespace

ElegooLink::ElegooLink()
{
    elink::ElegooLink::Config cfg;

    cfg.logLevel         = 2;
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
        std::cerr << "Error initializing ElegooLink" << std::endl;
    }
    std::string version = elink::ElegooLink::getInstance().getVersion();
    wxLogMessage("ElegooLink version: %s", version.c_str());
    // Subscribe to ElegooLink events
    elink::ElegooLink::getInstance().subscribeEvent<elink::DeviceConnectionEvent>(
        [&](const std::shared_ptr<elink::DeviceConnectionEvent>& event) {
            PrinterConnectStatus status = (event->connectionStatus.status == elink::ConnectionStatus::CONNECTED) ?
                                              PRINTER_CONNECT_STATUS_CONNECTED :
                                              PRINTER_CONNECT_STATUS_DISCONNECTED;

            PrinterNetworkEvent::getInstance()->connectStatusChanged.emit(
                PrinterConnectStatusEvent(event->connectionStatus.deviceId, status));
        });

    elink::ElegooLink::getInstance().subscribeEvent<elink::DeviceStatusEvent>([&](const std::shared_ptr<elink::DeviceStatusEvent>& event) {
        
        PrinterStatus status = parseElegooStatus(event->status.machineStatus.status, event->status.machineStatus.subStatus);
        PrinterNetworkEvent::getInstance()->statusChanged.emit(PrinterStatusEvent(event->status.deviceId, status));


        PrinterPrintTask task;
        task.taskId        = event->status.printStatus.taskId;
        task.fileName      = event->status.printStatus.fileName;
        task.totalTime     = event->status.printStatus.totalTime;
        task.currentTime   = event->status.printStatus.currentTime;
        task.estimatedTime = event->status.printStatus.estimatedTime;
        task.progress      = event->status.printStatus.progress;

        PrinterNetworkEvent::getInstance()->printTaskChanged.emit(PrinterPrintTaskEvent(event->status.deviceId, task));
    });

    elink::ElegooLink::getInstance().subscribeEvent<elink::DeviceAttributesEvent>([&](const std::shared_ptr<elink::DeviceAttributesEvent>& event) {
        PrinterNetworkInfo info;
        info.printerId       = event->attributes.deviceId;
        info.firmwareVersion = event->attributes.firmwareVersion;
        PrinterNetworkEvent::getInstance()->attributesChanged.emit(PrinterAttributesEvent(event->attributes.deviceId, info));
    });

    mIsCleanup = false;
}

ElegooLink::~ElegooLink()
{
    close();    
}

void ElegooLink::close()
{
    //only cleanup once
    std::lock_guard<std::mutex> lock(mMutex);
    if(!mIsCleanup) {
        mIsCleanup = true;
        elink::ElegooLink::getInstance().cleanup(); 
    }
}

PrinterNetworkResult<PrinterNetworkInfo> ElegooLink::addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    connected = false;
    try {
        auto params  = setupConnectionParams(printerNetworkInfo, false);
        auto elinkResult = elink::ElegooLink::getInstance().connectDevice(params);
        resultCode    = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ElegooError::SUCCESS) {
            if (elinkResult.data.has_value() && elinkResult.data.value().isConnected && elinkResult.data.value().deviceInfo.deviceId == printerNetworkInfo.printerId) {      
                auto addPrinter = elinkResult.data.value().deviceInfo;
                connected = true;
                PrinterNetworkInfo info = printerNetworkInfo;
                info.firmwareVersion = addPrinter.firmwareVersion;
                info.webUrl          = addPrinter.webUrl;
                info.connectionUrl   = addPrinter.connectionUrl;
                return PrinterNetworkResult<PrinterNetworkInfo>(resultCode, info);
            } else {
                resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_INVALID_DATA;
            }
        }      
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::addPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<PrinterNetworkInfo>(resultCode,  printerNetworkInfo);
}


PrinterNetworkResult<PrinterNetworkInfo> ElegooLink::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    try {
        auto params      = setupConnectionParams(printerNetworkInfo, true);
        auto elinkResult = elink::ElegooLink::getInstance().connectDevice(params);
        resultCode    = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ElegooError::SUCCESS) {
            if (elinkResult.data.has_value() && elinkResult.data.value().isConnected && elinkResult.data.value().deviceInfo.deviceId == printerNetworkInfo.printerId) {      
                auto addPrinter = elinkResult.data.value().deviceInfo;
                PrinterNetworkInfo info = printerNetworkInfo;
                info.firmwareVersion = addPrinter.firmwareVersion;
                info.webUrl          = addPrinter.webUrl;
                info.connectionUrl   = addPrinter.connectionUrl;
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
        auto elinkResult = elink::ElegooLink::getInstance().disconnectDevice(printerId);
        resultCode    = parseElegooResult(elinkResult.code);

    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::disconnectFromPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS);
}


PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooLink::discoverDevices()
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    std::vector<PrinterNetworkInfo> discoverDevices;
    try {
        elink::DeviceDiscoveryParams discoveryParams;
        discoveryParams.timeoutMs         = 5 * 1000;
        discoveryParams.broadcastInterval = 1000;
        discoveryParams.enableAutoRetry   = true;
        auto elinkResult = elink::ElegooLink::getInstance().startDeviceDiscovery(discoveryParams);
        resultCode  = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ElegooError::SUCCESS && elinkResult.data.has_value()) {
            for (const auto& device : elinkResult.value().devices) {
                PrinterNetworkInfo info;
                info.printerName     = device.name;
                info.host            = device.host;
                info.printerModel    = device.model;
                info.firmwareVersion = device.firmwareVersion;
                info.serialNumber    = device.serialNumber;
                info.vendor          = device.brand;
                info.deviceType      = static_cast<int>(device.deviceType);
                info.webUrl          = device.webUrl;
                info.connectionUrl   = device.connectionUrl;
                info.authMode        = device.authMode;
                info.mainboardId     = device.mainboardId;
                discoverDevices.push_back(info);
            }
        } 
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::discoverDevices: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(resultCode, discoverDevices);
}

bool ElegooLink::isBusy(const std::string& printerId, PrinterStatus &status)
{
    bool isBusy = true;
    status = PRINTER_STATUS_UNKNOWN;
    for(int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elinkResult = elink::ElegooLink::getInstance().getDeviceStatus({printerId});
        if (elinkResult.code == elink::ElegooError::SUCCESS) {
            const auto& statusData = elinkResult.value();
            if(statusData.machineStatus.status == elink::MachineMainStatus::IDLE) {
                isBusy = false;
                status = PRINTER_STATUS_IDLE;
                break;
            } else {
                status = parseElegooStatus(statusData.machineStatus.status, statusData.machineStatus.subStatus);
            }
        } else {
            return false;
        }
    }
    return isBusy;
}

PrinterNetworkResult<bool> ElegooLink::sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    try {
        PrinterStatus status;
        if(isBusy(printerNetworkInfo.printerId, status)) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_BUSY, false, "Printer is busy, status: " + std::to_string(status));
        }
        elink::StartPrintParams startPrintParams;
        startPrintParams.deviceId        = printerNetworkInfo.printerId;
        startPrintParams.storageLocation = "local";
        startPrintParams.fileName        = params.fileName;
        startPrintParams.autoBedLeveling = params.heatedBedLeveling;
        startPrintParams.heatedBedType   = params.bedType;
        startPrintParams.enableTimeLapse = params.timeLapse;
        auto elinkResult = elink::ElegooLink::getInstance().startPrint(startPrintParams);
        resultCode  = parseElegooResult(elinkResult.code);
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::sendPrintTask: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS);
}


PrinterNetworkResult<bool> ElegooLink::sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    try {
        PrinterStatus status;
        if(isBusy(printerNetworkInfo.printerId, status)) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::PRINTER_BUSY, false, "Printer is busy, status: " + std::to_string(status));
        }
        elink::FileUploadParams uploadParams;
        uploadParams.deviceId = printerNetworkInfo.printerId;
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

int ElegooLink::getDeviceType(const PrinterNetworkInfo& printerNetworkInfo)
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
            return static_cast<int>(elink::DeviceType::ELEGOO_FDM_CC2);
        } else if(model == "elegoo centauri carbon" || model == "elegoo centauri") {
            return static_cast<int>(elink::DeviceType::ELEGOO_FDM_CC);  
        } else if (vendor == "elegoo") {
            return static_cast<int>(elink::DeviceType::ELEGOO_FDM_KLIPPER);
        } else {
            return static_cast<int>(elink::DeviceType::GENERIC_FDM_KLIPPER);
        }
    } 
    return printerNetworkInfo.deviceType; 
}

PrinterNetworkResult<PrinterMmsGroup> ElegooLink::getPrinterMmsInfo(const PrinterNetworkInfo& printerNetworkInfo)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::UNKNOWN_ERROR;
    PrinterMmsGroup mmsGroup;
    try {
        auto elinkResult = elink::ElegooLink::getInstance().GetCanvasStatus({printerNetworkInfo.printerId});
        resultCode = parseElegooResult(elinkResult.code);
        if(resultCode == PrinterNetworkErrorCode::SUCCESS) {
            if(elinkResult.hasData()) {
                const auto& mmsData = elinkResult.value();
                mmsGroup.activeMmsId = mmsData.activeCanvasId;
                mmsGroup.activeTrayId = mmsData.activeTrayId;
                mmsGroup.autoRefill = mmsData.autoRefill;

                for(const auto& canvas : mmsData.canvases) {
                    PrinterMms mmsInfo;
                    mmsInfo.mmsId = canvas.canvasId;
                    mmsInfo.connected = canvas.connected;
                    for(const auto& tray : canvas.trays) {
                        PrinterMmsTray trayInfo;
                        trayInfo.trayId = tray.trayId;
                        trayInfo.vendor = tray.brand;
                        trayInfo.filamentType = tray.filamentType;
                        trayInfo.filamentName = tray.filamentName;
                        trayInfo.filamentColor = tray.filamentColor;
                        trayInfo.minNozzleTemp = tray.minNozzleTemp;
                        trayInfo.maxNozzleTemp = tray.maxNozzleTemp;
                        trayInfo.status = tray.status;
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
} // namespace Slic3r