#ifndef slic3r_ElegooNetwork_hpp_
#define slic3r_ElegooNetwork_hpp_

#include "PrinterNetwork.hpp"
#include "WebSocketClient.hpp"
#include <wx/thread.h>
#include <atomic>
#include <queue>
#include <mutex>
#include <thread>
#include <iostream>

namespace Slic3r {

class ElegooLocalWebSocket : public WebSocketClient
{
public:
    ElegooLocalWebSocket();
    virtual ~ElegooLocalWebSocket();
    static ElegooLocalWebSocket* getInstance() {
        static ElegooLocalWebSocket instance;
        return &instance;
    }

    std::vector<std::map<std::string, std::string>> discoverDevices();

private:
    WebSocketClient m_websocket;
    std::vector<std::map<std::string, std::string>> m_printerList;
    std::mutex m_printerListMutex;
    std::thread m_discoverThread;
    std::atomic<bool> m_discoverRunning;
    std::atomic<bool> m_discoverFinished;
    std::atomic<bool> m_discoverError;
};

class ElegooNetwork : public IPrinterNetwork
{
public:
    ElegooNetwork();
    virtual ~ElegooNetwork();

    bool connect(const std::string& printerId, const std::string& printerIp, const std::string& printerPort) override;
    void disconnect() override;
    bool isConnected() const override;
    NetworkStatus getStatus() const override;
    
    bool sendPrintTask(const std::string& task) override;
    bool sendPrintFile(const std::string& file) override;
    
    std::vector<std::map<std::string, std::string>> discoverDevices() override;
    
    void setMessageCallback(MessageCallback callback) override;
    void setStatusCallback(StatusCallback callback) override;
    
    std::string getPrinterId() const override;
    NetworkConfig getConfig() const override;

private:
    std::string m_printerId;
    std::string m_printerIp;
    std::string m_printerPort;
    std::string m_printerName;
    std::string m_printerModel;
    std::string m_printerProtocolVersion;
    std::string m_printerFirmwareVersion;
    
    std::atomic<NetworkStatus> m_status;
    std::atomic<bool> m_connected;
    
    MessageCallback m_messageCallback;
    StatusCallback m_statusCallback;

};

} // namespace Slic3r

#endif
