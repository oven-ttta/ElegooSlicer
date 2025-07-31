#include "ElegooLink.hpp"
#include <nlohmann/json.hpp>
#include "elegoolink/elegoolink.h"

namespace Slic3r {

ElegooLink::ElegooLink() {
    elink::ElegooLink::Config cfg;
    cfg.level = 2;
    cfg.enableConsole = true;
    cfg.enableFile = false;

    if(!elink::ElegooLink::getInstance().initialize(cfg)) {
        std::cerr << "Error initializing ElegooLink" << std::endl;
    }  

    elink::ElegooLink::getInstance().subscribeEvent<elink::DeviceStatusEvent>([&](const std::shared_ptr<elink::DeviceStatusEvent>& event) {
        std::cout << "Device status: " << event->status.deviceId << " " << event->status.printStatus.progress << std::endl;
    });

    // elink::ElegooLink::getInstance().subscribeEvent<elink::ConnectionStatusEvent>([&](const std::shared_ptr<elink::ConnectionStatusEvent>& event) {
    //     std::cout << "Connection status: " << event->status.connectionStatus << std::endl;
    // });
}

ElegooLink::~ElegooLink() {
    elink::ElegooLink::getInstance().cleanup();
}

bool ElegooLink::addPrinter(const PrinterNetworkInfo& printerNetworkInfo) {
    
    elink::ConnectDeviceParams connectionParams;
    connectionParams.deviceId = printerNetworkInfo.deviceId;
    connectionParams.deviceType = (elink::DeviceType)printerNetworkInfo.deviceType;
    connectionParams.authMode = "basic";
    connectionParams.username = "elegoo";
    connectionParams.password = "123456";
    connectionParams.host = printerNetworkInfo.ip;
    connectionParams.model = printerNetworkInfo.machineModel;

    auto connectionResult = elink::ElegooLink::getInstance().connectDevice(connectionParams);
    if (connectionResult.code != elink::ElegooError::SUCCESS) {
        wxLogError("Error connecting to device: %s", connectionResult.message.c_str());
        return false;
    }
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    mPrinterList.push_back(printerNetworkInfo);
    return true;          
   
}

void ElegooLink::removePrinter(const PrinterNetworkInfo& printerNetworkInfo) {
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    for (auto it = mPrinterList.begin(); it != mPrinterList.end(); ++it) {
        if (it->id == printerNetworkInfo.id) {
            mPrinterList.erase(it);
            break;
        }
    }
}

bool ElegooLink::isPrinterConnected(const PrinterNetworkInfo& printerNetworkInfo) {
    std::lock_guard<std::mutex> lock(mPrinterListMutex);   
    for (const auto& printer : mPrinterList) {
        if (printer.id == printerNetworkInfo.id) {
            return true;
        }
    }
    return false;
}

std::vector<PrinterNetworkInfo> ElegooLink::discoverDevices() {
  
    std::vector<PrinterNetworkInfo> printerList;
    elink::DeviceDiscoveryParams discoveryParams;
    discoveryParams.timeoutMs = 5 * 1000;
    discoveryParams.broadcastInterval = 1000;   
    discoveryParams.enableAutoRetry = true;    
    
    auto discoveryResult = elink::ElegooLink::getInstance().startDeviceDiscovery(discoveryParams);
    if (discoveryResult.code != elink::ElegooError::SUCCESS) {
        wxLogError("Error discovering devices: %s %s", discoveryResult.message.c_str(), discoveryResult.code);
        return printerList;
    }


    for(const auto& device : discoveryResult.data->devices) {
        PrinterNetworkInfo printerInfo;
        printerInfo.id = device.deviceId;
        printerInfo.name = device.name;
        printerInfo.vendor = device.brand;
        printerInfo.connectionUrl = device.connectionUrl;
        printerInfo.deviceId = device.deviceId;
        printerInfo.deviceType = (int)device.deviceType;
        printerInfo.firmwareVersion = device.firmwareVersion;
        printerInfo.protocolVersion = device.firmwareVersion;
        printerInfo.ip = device.host;
        printerInfo.machineModel = device.model;
        printerInfo.machineName = device.name;
        printerInfo.serialNumber = device.serialNumber;
        printerInfo.webUrl = device.webUrl;
        printerInfo.port = 0;
        printerList.push_back(printerInfo);
    } 
    return printerList;
}

bool ElegooLink::sendPrintFile(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) {

   elink::FileUploadParams uploadParams;
   uploadParams.deviceId = printerNetworkInfo.deviceId;
   uploadParams.storageLocation = "local";
   uploadParams.localFilePath = params.filePath;
   uploadParams.fileName = params.fileName;

    auto result = elink::ElegooLink::getInstance().uploadFile(uploadParams, [&](const elink::FileUploadProgressData& progress) -> bool {
        if(params.uploadProgressFn) {
            bool cancel = false;
            params.uploadProgressFn(progress.uploadedBytes, progress.totalBytes, cancel);
            if(cancel) {
                return false;
            }
        }   
        return true; 
    });

    if (result.code == elink::ElegooError::SUCCESS) {
        std::cout << "File uploaded successfully" << std::endl;
    } else {
        wxLogError("Error uploading file: %s %s", result.message.c_str(), result.code);
        if(params.errorFn) {
            std::string errorMsg = std::to_string((int)result.code);
            params.errorFn(errorMsg);
        }
        return false;
    }
    return true;
}

bool ElegooLink::sendPrintTask(const PrinterNetworkInfo& printerNetworkInfo, const PrinterNetworkParams& params) {

    elink::StartPrintParams startPrintParams;
    startPrintParams.deviceId = printerNetworkInfo.deviceId;
    startPrintParams.storageLocation = "local";
    startPrintParams.fileName = params.fileName;
    startPrintParams.autoBedLeveling = params.heatedBedLeveling;
    startPrintParams.heatedBedType = params.bedType;
    startPrintParams.enableTimeLapse = params.timeLapse;

    auto result = elink::ElegooLink::getInstance().startPrint(startPrintParams);
    if (result.code == elink::ElegooError::SUCCESS) {
        return true;
    } 
    wxLogError("Error sending print task: %s %s", result.message.c_str(), result.code);
    return false;
}


} // namespace Slic3r 