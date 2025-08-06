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
    case elink::ElegooError::DEVICE_BUSY: return PrinterNetworkErrorCode::DEVICE_BUSY;
    case elink::ElegooError::DEVICE_OFFLINE: return PrinterNetworkErrorCode::DEVICE_OFFLINE;
    case elink::ElegooError::DEVICE_INITIALIZATION_ERROR: return PrinterNetworkErrorCode::DEVICE_INITIALIZATION_ERROR;
    case elink::ElegooError::DEVICE_COMMAND_FAILED: return PrinterNetworkErrorCode::DEVICE_COMMAND_FAILED;
    case elink::ElegooError::DEVICE_ALREADY_CONNECTED: return PrinterNetworkErrorCode::DEVICE_ALREADY_CONNECTED;
    case elink::ElegooError::DEVICE_INTERNAL_ERROR: return PrinterNetworkErrorCode::DEVICE_INTERNAL_ERROR;
    case elink::ElegooError::DEVICE_TASK_NOT_FOUND: return PrinterNetworkErrorCode::DEVICE_TASK_NOT_FOUND;
    default: return PrinterNetworkErrorCode::UNKNOWN_ERROR;
    }
}

elink::ConnectDeviceParams setupConnectionParams(const PrinterNetworkInfo& printerNetworkInfo, bool checkConnection = false)
{
    elink::ConnectDeviceParams connectionParams;
    connectionParams.deviceId        = printerNetworkInfo.printerId;
    connectionParams.deviceType      = (elink::DeviceType) printerNetworkInfo.deviceType;
    connectionParams.authMode        = printerNetworkInfo.authMode;
    connectionParams.username        = printerNetworkInfo.username;
    connectionParams.password        = printerNetworkInfo.password;
    connectionParams.host            = printerNetworkInfo.host;
    connectionParams.model           = printerNetworkInfo.printerModel;
    connectionParams.checkConnection = checkConnection;

    if (printerNetworkInfo.authMode == "basic") {
        connectionParams.username = "";
        connectionParams.password = "";
    } else if (printerNetworkInfo.authMode == "token") {
        // nlohmann::json extraInfo = nlohmann::json::parse(printerNetworkInfo.extraInfo);
        // if (extraInfo.contains(PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN)) {
        //     connectionParams.token = extraInfo[PRINTER_NETWORK_EXTRA_INFO_KEY_TOKEN].get<std::string>();
        // } else {
        //     wxLogError("Error connecting to device: %s", "Token is not set");
        // }
    }

    return connectionParams;
}
} // namespace

ElegooLink::ElegooLink()
{
    elink::ElegooLink::Config cfg;

    cfg.level         = 2;
    cfg.enableConsole = true;

    cfg.enableFile    = false;
    cfg.enableWebServer =true;
    cfg.webServerPort = 32538;
    auto webDir = resources_dir();
    std::replace(webDir.begin(), webDir.end(), '\\', '/');
    cfg.staticWebPath = webDir + "/web/elegoo-fdm-web";

    if (!elink::ElegooLink::getInstance().initialize(cfg)) {
        std::cerr << "Error initializing ElegooLink" << std::endl;
 
    }

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


        PrinterStatus status = PRINTER_STATUS_IDLE;
        switch (event->status.machineStatus.status) {
        case elink::MachineMainStatus::OFFLINE: status = PRINTER_STATUS_OFFLINE; break;
        case elink::MachineMainStatus::IDLE: status = PRINTER_STATUS_IDLE; break;
        case elink::MachineMainStatus::PRINTING: status = PRINTER_STATUS_PRINTING; break;
        case elink::MachineMainStatus::SELF_CHECKING: status = PRINTER_STATUS_SELF_CHECKING; break;
        case elink::MachineMainStatus::AUTO_LEVELING: status = PRINTER_STATUS_AUTO_LEVELING; break;
        case elink::MachineMainStatus::PID_CALIBRATING: status = PRINTER_STATUS_PID_CALIBRATING; break;
        case elink::MachineMainStatus::RESONANCE_TESTING: status = PRINTER_STATUS_RESONANCE_TESTING; break;
        case elink::MachineMainStatus::UPDATING: status = PRINTER_STATUS_UPDATING; break;
        case elink::MachineMainStatus::FILE_COPYING: status = PRINTER_STATUS_FILE_COPYING; break;
        case elink::MachineMainStatus::FILE_TRANSFERRING: status = PRINTER_STATUS_FILE_TRANSFERRING; break;
        case elink::MachineMainStatus::HOMING: status = PRINTER_STATUS_HOMING; break;
        case elink::MachineMainStatus::PREHEATING: status = PRINTER_STATUS_PREHEATING; break;
        case elink::MachineMainStatus::FILAMENT_OPERATING: status = PRINTER_STATUS_FILAMENT_OPERATING; break;
        case elink::MachineMainStatus::EXTRUDER_OPERATING: status = PRINTER_STATUS_EXTRUDER_OPERATING; break;
        case elink::MachineMainStatus::PRINT_COMPLETED: status = PRINTER_STATUS_PRINT_COMPLETED; break;
        case elink::MachineMainStatus::RFID_RECOGNIZING: status = PRINTER_STATUS_RFID_RECOGNIZING; break;
        case elink::MachineMainStatus::UNKNOWN: status = PRINTER_STATUS_UNKNOWN; break;
        case elink::MachineMainStatus::EXCEPTION: status = PRINTER_STATUS_ERROR; break;
        default: status = PRINTER_STATUS_UNKNOWN; break;
        }


        switch (event->status.machineStatus.subStatus) {
        case elink::MachineSubStatus::P_PAUSING: status = PRINTER_STATUS_PAUSING; break;
        case elink::MachineSubStatus::P_PAUSED: status = PRINTER_STATUS_PAUSED; break;
        default: break;
        }

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
}

ElegooLink::~ElegooLink()
{
    

}


PrinterNetworkResult<bool> ElegooLink::addPrinter(const PrinterNetworkInfo& printerNetworkInfo, bool& connected)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::SUCCESS;
    connected = false;
    try {
        auto params  = setupConnectionParams(printerNetworkInfo, false);
        auto elinkResult = elink::ElegooLink::getInstance().connectDevice(params);
        resultCode    = parseElegooResult(elinkResult.code);
        if (elinkResult.code == elink::ElegooError::SUCCESS) {
            if (elinkResult.data.value().isConnected) {
                connected = true;
            }
        }      
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::addPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }
    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS);
}


PrinterNetworkResult<bool> ElegooLink::connectToPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::SUCCESS;
    try {
        auto params      = setupConnectionParams(printerNetworkInfo, true);
        auto elinkResult = elink::ElegooLink::getInstance().connectDevice(params);
        resultCode    = parseElegooResult(elinkResult.code);
    } catch (const std::exception& e) {
        wxLogError("Exception in ElegooLink::connectToPrinter: %s", e.what());
        resultCode = PrinterNetworkErrorCode::PRINTER_NETWORK_EXCEPTION;
    }

    return PrinterNetworkResult<bool>(resultCode, resultCode == PrinterNetworkErrorCode::SUCCESS);
}


PrinterNetworkResult<bool> ElegooLink::disconnectFromPrinter(const std::string& printerId)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::SUCCESS;
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
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::SUCCESS;
    std::vector<PrinterNetworkInfo> discoverDevices;
    try {
        elink::DeviceDiscoveryParams discoveryParams;
        discoveryParams.timeoutMs         = 5 * 1000;
        discoveryParams.broadcastInterval = 1000;
        discoveryParams.enableAutoRetry   = true;
        auto elinkResult = elink::ElegooLink::getInstance().startDeviceDiscovery(discoveryParams);
        resultCode    = parseElegooResult(elinkResult.code);


        if (elinkResult.code == elink::ElegooError::SUCCESS && elinkResult.data.has_value()) {
            for (const auto& device : elinkResult.value().devices) {
                PrinterNetworkInfo info;
                info.printerId       = device.deviceId;
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


PrinterNetworkResult<bool> ElegooLink::sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params)
{
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::SUCCESS;
    try {
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
    PrinterNetworkErrorCode resultCode = PrinterNetworkErrorCode::SUCCESS;
    try {
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
    if(resultCode != PrinterNetworkErrorCode::SUCCESS && params.errorFn) {
        params.errorFn(networkResult.message);
    }   
    return networkResult;
}


} // namespace Slic3r