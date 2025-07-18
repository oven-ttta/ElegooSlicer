#include "ElegooNetwork.hpp"
#include <wx/log.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Slic3r {

ElegooNetwork::ElegooNetwork(){
    m_status = NetworkStatus::DISCONNECTED;
    m_connected = false;
}

ElegooNetwork::~ElegooNetwork() {
    disconnect();
}

bool ElegooNetwork::connect(const std::string& printerId, const std::string& printerIp, const std::string& printerPort)
{
    m_printerId = printerId;
    m_printerIp = printerIp;
    m_printerPort = printerPort;
    return true;

}

void ElegooNetwork::disconnect()
{

}

bool ElegooNetwork::isConnected() const
{
    return true;
}

NetworkStatus ElegooNetwork::getStatus() const
{
    return m_status;
}

std::vector<std::map<std::string, std::string>> ElegooNetwork::discoverDevices()   
{
    std::vector<std::map<std::string, std::string>> printerList;
    printerList = ElegooLocalWebSocket::getInstance()->discoverDevices();
    return printerList;
}

bool ElegooNetwork::sendPrintTask(const std::string& task)
{
   return true;
}

bool ElegooNetwork::sendPrintFile(const std::string& file)
{
   return false;
}

void ElegooNetwork::setMessageCallback(MessageCallback callback)
{
    m_messageCallback = callback;
}

void ElegooNetwork::setStatusCallback(StatusCallback callback)
{
    m_statusCallback = callback;
}

std::string ElegooNetwork::getPrinterId() const
{
    return m_printerId;
}

NetworkConfig ElegooNetwork::getConfig() const
{
    NetworkConfig config;
    config.ip = m_printerIp;
    config.port = std::stoi(m_printerPort);
    config.vendor = "Elegoo";
    config.protocol = "websocket";
    config.timeout = 30;
    config.useSSL = false;
    return config;
}

ElegooLocalWebSocket::ElegooLocalWebSocket()
{
    m_websocket.connect("10.31.3.249", "6382", "/ws");
}

ElegooLocalWebSocket::~ElegooLocalWebSocket()
{

}

std::vector<std::map<std::string, std::string>> ElegooLocalWebSocket::discoverDevices()
{
    nlohmann::json message = nlohmann::json::object();
    message["id"] = "msg_002";
    message["version"] = "1.0";
    message["method"] = 20;
    message["data"] = nlohmann::json::object();
    message["data"]["timeoutMs"] = 3000;
    message["data"]["broadcastInterval"] = 2;
    message["data"]["enableAutoRetry"] = false;
    message["data"]["preferredListenPorts"] = {8080, 8081};
    
    std::string messageStr = message.dump();
    m_websocket.send(messageStr);
    std::string response = m_websocket.receive();
    
    std::vector<std::map<std::string, std::string>> printerList;
    
    try {
        nlohmann::json responseJson = nlohmann::json::parse(response);
        
        if (responseJson.contains("data") && responseJson["data"].contains("code")) {
            int code = responseJson["data"]["code"];
            if (code == 0 && responseJson["data"].contains("devices")) {
                auto devices = responseJson["data"]["devices"];
                for (const auto& device : devices) {
                    std::map<std::string, std::string> printerInfo;
                    printerInfo["id"] = device.value("id", "");
                    printerInfo["name"] = device.value("name", "");
                    printerInfo["authMode"] = device.value("authMode", "");
                    printerInfo["vendor"] = device.value("brand", "");
                    printerInfo["connectionUrl"] = device.value("connectionUrl", "");
                    printerInfo["deviceId"] = device.value("deviceId", "");
                    printerInfo["deviceType"] = std::to_string(device.value("deviceType", 0));
                    printerInfo["firmwareVersion"] = device.value("firmwareVersion", "");
                    printerInfo["protocolVersion"] = device.value("firmwareVersion", "");
                    printerInfo["ip"] = device.value("ipAddress", "");
                    printerInfo["manufacturer"] = device.value("manufacturer", "");
                    printerInfo["machineModel"] = device.value("model", "");
                    printerInfo["machineName"] = device.value("name", "");
                    printerInfo["protocolType"] = std::to_string(device.value("protocolType", 1));
                    printerInfo["serialNumber"] = device.value("serialNumber", "");
                    printerInfo["webUrl"] = device.value("webUrl", "");     
                    printerInfo["port"] = "3030";
                    printerList.push_back(printerInfo);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing device discovery response: " << e.what() << std::endl;
    }
    
    return printerList;
}

} // namespace Slic3r 


