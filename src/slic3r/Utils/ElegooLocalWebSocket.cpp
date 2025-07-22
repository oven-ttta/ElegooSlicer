#include "ElegooLocalWebSocket.hpp"
#include "WSManager.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>

namespace Slic3r {

ElegooHandler::ElegooHandler(const std::string& clientId, const std::string& clientName, const std::string& host, const std::string& port)
    : mClientId(clientId)
    , mClientName(clientName)
    , mHost(host)
    , mPort(port)
{
}

ElegooMessageHandler::ElegooMessageHandler(const std::string& clientId, const std::string& clientName, const std::string& host, const std::string& port)
    : ElegooHandler(clientId, clientName, host, port)
{
}

ElegooStatusHandler::ElegooStatusHandler(const std::string& clientId, const std::string& clientName, const std::string& host, const std::string& port)
    : ElegooHandler(clientId, clientName, host, port)
{
}

std::string ElegooMessageHandler::getRequestID(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        
        if (json.contains("id")) {
            std::string requestId = json["id"];
            
            requestId.erase(std::remove_if(requestId.begin(), requestId.end(), 
                [](char c) { return c < 32 || c == 127; }), requestId.end());
            
            return requestId;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing request ID: " << e.what() << std::endl;
    }
    
    return "";
}

bool ElegooMessageHandler::isResponse(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        
        if(json.contains("messageType") && json["messageType"] == "response") {
            return true;
        }
        if (json.contains("id")) {
            std::string id = json["id"];
            if (id.rfind("req_", 0) == 0) {
                return true;
            }
        }
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking response: " << e.what() << std::endl;
    }
    
    return false;
}

bool ElegooMessageHandler::isReport(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        if(json.contains("messageType") && json["messageType"] == "notification") {
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error checking report: " << e.what() << std::endl;
    }
    
    return false;
}

void ElegooMessageHandler::handleReport(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        if(json.contains("method")) {
            int method = json["method"];
            if(method == 2000) {
                handleStatusReport(data);
            } else if(method == 2001) {
                handleAttributesReport(data);
            } else if(method == 2002) {
                handleErrorReport(data);
            } else if(method == 2003) {
                handleConnectionStatusReport(data);
            } else if(method == 2004) {
                handleFileTransferProgress(data);
            } else if(method == 2005) {
                handleDeviceDiscovery(data);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling report: " << e.what() << std::endl;
    }
}

void ElegooStatusHandler::onStatusChange(const std::string& clientId, WSStatus status, const std::string& error) {
    if (status == WSStatus::Connected) {
        std::cout << "ElegooLocalWebSocket connected" << std::endl;
    } else if (status == WSStatus::Disconnected) {
        std::cout << "ElegooLocalWebSocket disconnected" << std::endl;
    }
}

void ElegooMessageHandler::handleStatusReport(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        std::cout << "=== Device Status Report ===" << std::endl;
        
        if (json.contains("data")) {
            auto dataObj = json["data"];
            
            // Device ID
            if (dataObj.contains("deviceId")) {
                std::cout << "Device ID: " << dataObj["deviceId"] << std::endl;
            }
            
            // Temperatures
            if (dataObj.contains("temperatures")) {
                auto temps = dataObj["temperatures"];
                std::cout << "Temperatures:" << std::endl;
                
                if (temps.contains("hotend")) {
                    auto hotend = temps["hotend"];
                    std::cout << "  Hotend: " << hotend.value("current", 0.0) 
                              << "°C (target: " << hotend.value("target", 0.0) << "°C)" << std::endl;
                }
                
                if (temps.contains("bed")) {
                    auto bed = temps["bed"];
                    std::cout << "  Bed: " << bed.value("current", 0.0) 
                              << "°C (target: " << bed.value("target", 0.0) << "°C)" << std::endl;
                }
            }
            
            // Fans
            if (dataObj.contains("fans")) {
                auto fans = dataObj["fans"];
                std::cout << "Fans:" << std::endl;
                
                for (auto it = fans.begin(); it != fans.end(); ++it) {
                    std::string fanName = it.key();
                    auto fan = it.value();
                    std::cout << "  " << fanName << ": " << fan.value("speed", 0) 
                              << "% (" << fan.value("rpm", 0) << " RPM)" << std::endl;
                }
            }
            
            // Machine Status
            if (dataObj.contains("machineStatus")) {
                auto status = dataObj["machineStatus"];
                std::cout << "Machine Status: " << status.value("status", -1) 
                          << " (Sub: " << status.value("subStatus", 0) << ")" << std::endl;
                std::cout << "Progress: " << status.value("progress", 0) << "%" << std::endl;
            }
            
            // Print Status
            if (dataObj.contains("printStatus")) {
                auto print = dataObj["printStatus"];
                std::cout << "Print Status:" << std::endl;
                std::cout << "  File: " << print.value("filename", "") << std::endl;
                std::cout << "  Progress: " << print.value("progress", 0.0) << "%" << std::endl;
                std::cout << "  Layer: " << print.value("currentLayer", 0) 
                          << "/" << print.value("totalLayer", 0) << std::endl;
                std::cout << "  Time: " << print.value("currentTime", 0) 
                          << "/" << print.value("totalTime", 0) << "s" << std::endl;
            }
        }
        std::cout << "===========================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling status report: " << e.what() << std::endl;
    }
}

void ElegooMessageHandler::handleAttributesReport(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        std::cout << "=== Device Attributes Report ===" << std::endl;
        
        if (json.contains("data")) {
            auto dataObj = json["data"];
            
            if (dataObj.contains("deviceId")) {
                std::cout << "Device ID: " << dataObj["deviceId"] << std::endl;
            }
            
            if (dataObj.contains("capabilities")) {
                auto caps = dataObj["capabilities"];
                std::cout << "Device Capabilities:" << std::endl;
                
                // Storage components
                if (caps.contains("storageComponents")) {
                    std::cout << "  Storage Components: ";
                    for (const auto& storage : caps["storageComponents"]) {
                        std::cout << storage.value("name", "") << " ";
                    }
                    std::cout << std::endl;
                }
                
                // Temperature components
                if (caps.contains("temperatureComponents")) {
                    std::cout << "  Temperature Components: ";
                    for (const auto& temp : caps["temperatureComponents"]) {
                        std::cout << temp.value("name", "") << " ";
                    }
                    std::cout << std::endl;
                }
                
                // Fan components
                if (caps.contains("fanComponents")) {
                    std::cout << "  Fan Components: ";
                    for (const auto& fan : caps["fanComponents"]) {
                        std::cout << fan.value("name", "") << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
        std::cout << "=================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling attributes report: " << e.what() << std::endl;
    }
}

void ElegooMessageHandler::handleErrorReport(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        std::cout << "=== Device Error Report ===" << std::endl;
        
        if (json.contains("data")) {
            auto dataObj = json["data"];
            
            if (dataObj.contains("deviceId")) {
                std::cout << "Device ID: " << dataObj["deviceId"] << std::endl;
            }
            
            if (dataObj.contains("errorCode")) {
                std::cout << "Error Code: " << dataObj["errorCode"] << std::endl;
            }
            
            if (dataObj.contains("errorMessage")) {
                std::cout << "Error Message: " << dataObj["errorMessage"] << std::endl;
            }
            
            if (dataObj.contains("exceptionStatus")) {
                auto exceptions = dataObj["exceptionStatus"];
                if (!exceptions.empty()) {
                    std::cout << "Exception Status: ";
                    for (const auto& ex : exceptions) {
                        std::cout << ex.get<int>() << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
        std::cout << "============================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling error report: " << e.what() << std::endl;
    }
}

void ElegooMessageHandler::handleConnectionStatusReport(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        std::cout << "=== Connection Status Report ===" << std::endl;
        
        if (json.contains("data")) {
            auto dataObj = json["data"];
            
            if (dataObj.contains("deviceId")) {
                std::cout << "Device ID: " << dataObj["deviceId"] << std::endl;
            }
            
            if (dataObj.contains("connectionStatus")) {
                int status = dataObj["connectionStatus"];
                std::string statusStr = (status == 0) ? "Disconnected" : "Connected";
                std::cout << "Connection Status: " << statusStr << " (" << status << ")" << std::endl;
            }
            
            if (dataObj.contains("errorMessage")) {
                std::cout << "Error Message: " << dataObj["errorMessage"] << std::endl;
            }
        }
        std::cout << "================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling connection status report: " << e.what() << std::endl;
    }
}

void ElegooMessageHandler::handleFileTransferProgress(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        if (json.contains("data")) {
            auto dataObj = json["data"];
            std::string req_id = json.value("id", "");
            std::string device_id = dataObj.value("deviceId", "");
            std::string fileName = dataObj.value("fileName", "");
            double percentage = dataObj.value("percentage", 0.0);
            uint64_t transferred = dataObj.value("transferredBytes", 0);
            uint64_t total = dataObj.value("totalBytes", 0);
            double speed = dataObj.value("transferSpeed", 0.0);
            std::string errorMsg = dataObj.value("errorMessage", "");

            UploadProgressInfo info;
            info.reqId = req_id;
            info.deviceId = device_id;
            info.fileName = fileName;
            info.transferredBytes = transferred;
            info.totalBytes = total;
            info.speedKBps = speed / 1024.0;
            info.finished = (percentage >= 100.0);
            info.errorMsg = errorMsg;
            ElegooLocalWebSocket::instance()->setFileTransferProgress(req_id, device_id, info);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling file transfer progress: " << e.what() << std::endl;
    }
}

void ElegooMessageHandler::handleDeviceDiscovery(const std::string& data) {
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        std::cout << "=== Device Discovery Report ===" << std::endl;
        
        if (json.contains("data")) {
            auto dataObj = json["data"];
            
            if (dataObj.contains("devices")) {
                auto devices = dataObj["devices"];
                std::cout << "Discovered " << devices.size() << " device(s):" << std::endl;
                
                for (const auto& device : devices) {
                    std::cout << "  - " << device.value("name", "Unknown") 
                              << " (" << device.value("deviceId", "Unknown") << ")" << std::endl;
                    std::cout << "    IP: " << device.value("ipAddress", "Unknown") << std::endl;
                    std::cout << "    Model: " << device.value("model", "Unknown") << std::endl;
                    std::cout << "    Firmware: " << device.value("firmwareVersion", "Unknown") << std::endl;
                }
            }
        }
        std::cout << "===============================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling device discovery: " << e.what() << std::endl;
    }
}

ElegooLocalWebSocket::ElegooLocalWebSocket()
    : mClientId("elegoo_local_websocket")
    , mHost("127.0.0.1")
    , mPort("6382")
    , mStopMonitoring(false)
    , mIsConnected(false)
{
    mMessageHandler = std::make_shared<ElegooMessageHandler>(mClientId, "ElegooLocalWebSocket", mHost, mPort);
    mStatusHandler = std::make_shared<ElegooStatusHandler>(mClientId, "ElegooLocalWebSocket", mHost, mPort);

    if (connect()) {
        mIsConnected = true;
    }

    startMonitoring();
}

ElegooLocalWebSocket::~ElegooLocalWebSocket() {
    stopMonitoring();
    disconnect();
}

bool ElegooLocalWebSocket::connect() {
    if (mIsConnected.load()) {
        return true;
    }

    if (WSManager::instance()->addClient(mClientId, "ws://" + mHost + ":" + mPort + "/ws", mMessageHandler, mStatusHandler)) {
        mIsConnected = true;
        return true;
    }
    return false;
}

void ElegooLocalWebSocket::disconnect() {
    if (mIsConnected.load()) {
        WSManager::instance()->removeClient(mClientId);
        mIsConnected = false;
    }
}

void ElegooLocalWebSocket::startMonitoring() {
    if (mStopMonitoring.load()) {
        return;
    }
    mStopMonitoring.store(false);
    mMonitorThread = std::thread(&ElegooLocalWebSocket::monitorConnection, this);
    mMonitorThread.detach();
}

void ElegooLocalWebSocket::stopMonitoring() {
    mStopMonitoring = true;
    if (mMonitorThread.joinable()) {
        mMonitorThread.join();
    }
}

void ElegooLocalWebSocket::monitorConnection() {
    while (!mStopMonitoring) {
        try {
            if (!mIsConnected.load()) {
                if (connect()) {
                    mIsConnected = true;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "ElegooLocalWebSocket monitoring error: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
 }

std::string ElegooLocalWebSocket::generateRequestId() {
    static std::atomic<int> requestIdCounter(0);
    return "req_" + std::to_string(requestIdCounter++);
}

bool ElegooLocalWebSocket::addPrinter(const PrinterInfo& printerInfo) {
    if (!mIsConnected.load()) {
        return false;
    }
    
    nlohmann::json message = nlohmann::json::object();
    message["id"] = generateRequestId();
    message["version"] = "1.0";
    message["method"] = 1000;
    message["data"] = nlohmann::json::object();
    message["data"]["deviceId"] = printerInfo.deviceId;
    message["data"]["username"] = "elegoo";
    message["data"]["password"] = "123456";
    message["data"]["token"] = "";
    message["data"]["extraParams"] = nlohmann::json::object();
    std::string messageStr = message.dump();
    std::string response = WSManager::instance()->send(mClientId, messageStr);

    nlohmann::json responseJson = nlohmann::json::parse(response);
    if (responseJson.contains("data") && responseJson["data"].contains("code")) {
        int code = responseJson["data"]["code"];
        if (code == 0) {
            std::lock_guard<std::mutex> lock(mPrinterListMutex);
            mPrinterList.push_back(printerInfo);
            return true;    
        }
    }
    return false;
}

void ElegooLocalWebSocket::removePrinter(const PrinterInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(mPrinterListMutex);
    for (auto it = mPrinterList.begin(); it != mPrinterList.end(); ++it) {
        if (it->id == printerInfo.id) {
            mPrinterList.erase(it);
            break;
        }
    }
}

bool ElegooLocalWebSocket::isPrinterConnected(const PrinterInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(mPrinterListMutex);   
    for (const auto& printer : mPrinterList) {
        if (printer.id == printerInfo.id) {
            return true;
        }
    }
    return false;
}

std::vector<PrinterInfo> ElegooLocalWebSocket::discoverDevices() {
    if (!mIsConnected.load()) {
        return std::vector<PrinterInfo>(); 
    }
    
    nlohmann::json message = nlohmann::json::object();
    message["id"] = generateRequestId();
    message["version"] = "1.0";
    message["method"] = 20;
    message["data"] = nlohmann::json::object();
    message["data"]["timeoutMs"] = 3000;
    message["data"]["broadcastInterval"] = 2;
    message["data"]["enableAutoRetry"] = false;
    message["data"]["preferredListenPorts"] = {8080, 8081};
    
    std::string messageStr = message.dump();
    std::string response = WSManager::instance()->send(mClientId, messageStr);
    
    std::vector<PrinterInfo> printerList;
    
    try {
        nlohmann::json responseJson = nlohmann::json::parse(response);
        
        if (responseJson.contains("data") && responseJson["data"].contains("code")) {
            int code = responseJson["data"]["code"];
            if (code == 0 && responseJson["data"].contains("devices")) {
                auto devices = responseJson["data"]["devices"];
                for (const auto& device : devices) {
                    PrinterInfo printerInfo;
                    printerInfo.id = device.value("id", "");
                    printerInfo.name = device.value("name", "");
                    printerInfo.vendor = device.value("brand", "");
                    printerInfo.connectionUrl = device.value("connectionUrl", "");
                    printerInfo.deviceId = device.value("deviceId", "");
                    printerInfo.deviceType = std::to_string(device.value("deviceType", 0));
                    printerInfo.firmwareVersion = device.value("firmwareVersion", "");
                    printerInfo.protocolVersion = device.value("firmwareVersion", "");
                    printerInfo.ip = device.value("ipAddress", "");
                    printerInfo.machineModel = device.value("model", "");
                    printerInfo.machineName = device.value("name", "");
                    printerInfo.serialNumber = device.value("serialNumber", "");
                    printerInfo.webUrl = device.value("webUrl", "");     
                    printerInfo.port = "";
                    printerList.push_back(printerInfo);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing device discovery response: " << e.what() << std::endl;
    }
    
    return printerList;
}

bool ElegooLocalWebSocket::sendPrintFile(const PrinterInfo& printerInfo, const PrinterNetworkParams& params) {
    if (!mIsConnected.load()) {
        return false;
    }
    nlohmann::json message = nlohmann::json::object();
    message["id"] = generateRequestId();
    message["version"] = "1.0";
    message["method"] = 1300;
    message["data"] = nlohmann::json::object();
    message["data"]["deviceId"] = printerInfo.deviceId;
    message["data"]["storageLocation"] = "local";
    message["data"]["localFilePath"] = params.uploadData.source_path.string();
    message["data"]["fileName"] = params.uploadData.upload_path.filename().string();
    message["data"]["overwriteExisting"] = true;
    message["data"]["timeout"] = 600;
    message["data"]["extraHeaders"] = nlohmann::json::object();
    message["data"]["extraParams"] = nlohmann::json::object();
    std::string messageStr = message.dump();

    std::ifstream   file(params.uploadData.source_path.string(), std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.close();
    const std::string  fileSize = std::to_string(size);

    UploadProgressInfo info;
    info.reqId = message["id"];
    info.deviceId = printerInfo.deviceId;
    info.fileName = params.uploadData.upload_path.filename().string();
    info.totalBytes = size;
    info.transferredBytes = 0;
    info.finished = false;
    info.errorMsg = "";
    setFileTransferProgress(info.reqId, info.deviceId, info);
    int code = -1;
    bool cancel = false;
    std::atomic<bool> threadFinished{false};
    std::thread uploadThread([&]() {
        std::string response = WSManager::instance()->send(mClientId, messageStr, 3600 * 1000);
        nlohmann::json responseJson = nlohmann::json::parse(response);
        std::string errorMsg;
        if (responseJson.contains("data") && responseJson["data"].contains("code")) {
            code = responseJson["data"]["code"];
            if (code != 0) {        
                errorMsg = responseJson["data"].value("message", "");
            }
        } else {
            errorMsg = "unknown error";
        }
        info.finished = true;
        info.errorMsg = errorMsg;
        threadFinished = true;
    });

    UploadProgressInfo progressInfo = getUploadProgress(info.reqId, info.deviceId);
    while(!progressInfo.finished && !threadFinished) {
        if (params.progressFn) {
            Http::Progress progress = {
                progressInfo.totalBytes,
                progressInfo.transferredBytes,
                progressInfo.totalBytes,
                progressInfo.transferredBytes,
                "",
                progressInfo.speedKBps
            };
            params.progressFn(progress, cancel);
            if(cancel) {
                break;
            }
        }        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        progressInfo = getUploadProgress(info.reqId, info.deviceId);
    }

    if (uploadThread.joinable()) {
        uploadThread.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(mProgressMapMutex);
        std::string key = info.reqId + "_" + info.deviceId;
        mPrintersUploadProgress.erase(key);
    }
    if(cancel) {
        return false;
    }
    if(info.errorMsg != "") {
        if(params.errorFn) {
            params.errorFn(wxString::FromUTF8(info.errorMsg));
        }
        return false;
    }
    if(params.progressFn) {
        if(progressInfo.finished) {
            info = progressInfo;
        } 
        info.transferredBytes = info.totalBytes;
        Http::Progress progress = {
            info.totalBytes,
            info.transferredBytes,
            info.totalBytes,
            info.transferredBytes,
            "",
            info.speedKBps
        };
        params.progressFn(progress, cancel);
    }
    return code == 0;
}

bool ElegooLocalWebSocket::sendPrintTask(const PrinterInfo& printerInfo, const PrinterNetworkParams& params) {
    if (!mIsConnected.load()) {
        return false;
    }
    
    bool timeLapse = false;
    bool heatedBedLeveling = false;
    std::string bedType = "0"; 
    auto timeLapseIt = params.uploadData.extended_info.find("timeLapse");
    if (timeLapseIt != params.uploadData.extended_info.end()) {
        timeLapse = (timeLapseIt->second == "1");
    }
    
    auto heatedBedLevelingIt = params.uploadData.extended_info.find("heatedBedLeveling");
    if (heatedBedLevelingIt != params.uploadData.extended_info.end()) {
        heatedBedLeveling = (heatedBedLevelingIt->second == "1");
    }
    
    auto bedTypeIt = params.uploadData.extended_info.find("bedType");
    if (bedTypeIt != params.uploadData.extended_info.end()) {
        bedType = bedTypeIt->second;
    }

    nlohmann::json message = nlohmann::json::object();
    message["id"] = generateRequestId();
    message["version"] = "1.0";
    message["method"] = 1100;
    message["data"] = nlohmann::json::object();
    message["data"]["deviceId"] = printerInfo.deviceId;
    message["data"]["storageLocation"] = "local";
    message["data"]["fileName"] = params.uploadData.upload_path.filename().string();
    message["data"]["autoBedLeveling"] = heatedBedLeveling;
   
    if (bedType == std::to_string((int)BedType::btPC)){
        bedType = "1";
    }else{
        bedType = "0";
    }
    message["data"]["heatedBedType"] = std::stoi(bedType);
    message["data"]["enableTimeLapse"] = timeLapse;
    message["data"]["extraParams"] = nlohmann::json::object();
    std::string messageStr = message.dump();

    std::string response = WSManager::instance()->send(mClientId, messageStr);
    nlohmann::json responseJson = nlohmann::json::parse(response);

    if (responseJson.contains("data") && responseJson["data"].contains("code")) {
        int code = responseJson["data"]["code"];
        if(code == 0) {
            return true;
        }
    }
    return false;
}

void ElegooLocalWebSocket::setFileTransferProgress(const std::string& req_id, const std::string& device_id, const UploadProgressInfo& info) {
    std::lock_guard<std::mutex> lock(mProgressMapMutex);
    std::string key = req_id + "_" + device_id;
    if(mPrintersUploadProgress.find(key) == mPrintersUploadProgress.end()) {
        mPrintersUploadProgress[key] = info;
    } else {
        const auto& oldInfo = mPrintersUploadProgress[key];
        if(oldInfo.finished) {
            return;
        }
        if(info.finished || oldInfo.transferredBytes <= info.transferredBytes) {
            mPrintersUploadProgress[key] = info;
        }
    }
}

UploadProgressInfo ElegooLocalWebSocket::getUploadProgress(const std::string& req_id, const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mProgressMapMutex);
    std::string key = req_id + "_" + device_id;
    auto it = mPrintersUploadProgress.find(key);
    if (it != mPrintersUploadProgress.end()) {
        return it->second;
    }
    return UploadProgressInfo();
}

} // namespace Slic3r 