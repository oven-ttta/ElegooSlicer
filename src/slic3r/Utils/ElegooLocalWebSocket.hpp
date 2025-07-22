#ifndef slic3r_ElegooLocalWebSocket_hpp_
#define slic3r_ElegooLocalWebSocket_hpp_

#include "WSTypes.hpp"
#include "PrinterNetwork.hpp"
#include "Singleton.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <condition_variable>
#include <map>
#include <chrono>

namespace Slic3r {

class WSManager;

struct UploadProgressInfo {
    std::string reqId;
    std::string deviceId;
    std::string fileName;
    bool finished{false};
    uint64_t transferredBytes{0};
    uint64_t totalBytes{0};
    double speedKBps{0.0};
    std::string errorMsg;
};
class ElegooHandler {
public:
    virtual ~ElegooHandler() = default;
    virtual std::string getClientId() const { return mClientId; }
    virtual std::string getClientName() const { return mClientName; }
    virtual std::string getHost() const { return mHost; }
    virtual std::string getPort() const { return mPort; }

    ElegooHandler(const std::string& clientId, const std::string& clientName, const std::string& host, const std::string& port);
    ElegooHandler() = delete;
    ElegooHandler(const ElegooHandler&) = delete;
    ElegooHandler& operator=(const ElegooHandler&) = delete;

protected:
    std::string mClientId;
    std::string mClientName;
    std::string mHost;
    std::string mPort;
};

class ElegooMessageHandler : public MessageHandler, public ElegooHandler {
public:
    virtual std::string getRequestID(const std::string& data) override;
    virtual bool isResponse(const std::string& data) override;
    virtual bool isReport(const std::string& data) override;
    virtual void handleReport(const std::string& data) override;

public:
    ElegooMessageHandler(const std::string& clientId, const std::string& clientName, const std::string& host, const std::string& port);
    virtual ~ElegooMessageHandler() = default;

    ElegooMessageHandler() = delete;
    ElegooMessageHandler(const ElegooMessageHandler&) = delete;
    ElegooMessageHandler& operator=(const ElegooMessageHandler&) = delete;

private:
    // Notification handlers based on method types
    void handleStatusReport(const std::string& data);
    void handleAttributesReport(const std::string& data);
    void handleErrorReport(const std::string& data);
    void handleConnectionStatusReport(const std::string& data);
    void handleFileTransferProgress(const std::string& data);
    void handleDeviceDiscovery(const std::string& data);
};

class ElegooStatusHandler : public ConnectionStatusHandler, public ElegooHandler {
public:
    virtual void onStatusChange(const std::string& clientId, WSStatus status, const std::string& error) override;
   
public:
    ElegooStatusHandler(const std::string& clientId, const std::string& clientName, const std::string& host, const std::string& port);
    virtual ~ElegooStatusHandler() = default;
    ElegooStatusHandler() = delete;
    ElegooStatusHandler(const ElegooStatusHandler&) = delete;
    ElegooStatusHandler& operator=(const ElegooStatusHandler&) = delete;
};

class ElegooLocalWebSocket : public Singleton<ElegooLocalWebSocket> {
    friend class Singleton<ElegooLocalWebSocket>;

public:
    ElegooLocalWebSocket();
    ~ElegooLocalWebSocket();

    bool connect();
    void disconnect();
    void startMonitoring();
    void stopMonitoring();
    void monitorConnection();

    bool addPrinter(const PrinterInfo& printerInfo);
    void removePrinter(const PrinterInfo& printerInfo);
    bool isPrinterConnected(const PrinterInfo& printerInfo);
    std::vector<PrinterInfo> discoverDevices();

    bool sendPrintFile(const PrinterInfo& printerInfo, const PrinterNetworkParams& params);
    bool sendPrintTask(const PrinterInfo& printerInfo, const PrinterNetworkParams& params);


    void setFileTransferProgress(const std::string& req_id, const std::string& device_id, const UploadProgressInfo& info);
    UploadProgressInfo getUploadProgress(const std::string& req_id, const std::string& device_id);

private:
    std::string mClientId;
    std::string mHost;
    std::string mPort;

    std::atomic<bool> mStopMonitoring;
    std::atomic<bool> mIsConnected;
    std::thread mMonitorThread;

    mutable std::mutex mPrinterListMutex;
    std::vector<PrinterInfo> mPrinterList;

    std::shared_ptr<ElegooMessageHandler> mMessageHandler;
    std::shared_ptr<ElegooStatusHandler> mStatusHandler;

    std::mutex mProgressMutex;
    std::condition_variable mProgressCV;
    double mLastUploadProgress = 0.0;
    std::string mLastUploadFile;

    std::mutex mProgressMapMutex; 
    std::map<std::string, UploadProgressInfo> mPrintersUploadProgress;

    std::string generateRequestId();


};

} // namespace Slic3r

#endif 