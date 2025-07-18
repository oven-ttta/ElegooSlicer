#ifndef slic3r_PrinterNetwork_hpp_
#define slic3r_PrinterNetwork_hpp_

#include <string>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>

namespace Slic3r {

enum class NetworkStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECT_ERROR,
    DISCONNECT_ERROR
};

enum class MessageType {
    STATUS_UPDATE,
    PRINT_PROGRESS
};

struct NetworkMessage {
    MessageType type;
    std::string printerId;
    nlohmann::json data;
    std::string timestamp;
};

struct NetworkConfig {
    std::string ip;
    int port;
    std::string vendor;
    std::string protocol;
    int timeout;
    bool useSSL;
};

using MessageCallback = std::function<void(const NetworkMessage&)>;
using StatusCallback = std::function<void(const std::string&, NetworkStatus)>;

class IPrinterNetwork {
public:
    virtual ~IPrinterNetwork() = default;

    virtual bool connect(const std::string& printerId, const std::string& printerIp, const std::string& printerPort) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual NetworkStatus getStatus() const = 0;

    virtual bool sendPrintTask(const std::string& task) = 0;
    virtual bool sendPrintFile(const std::string& file) = 0;

    virtual std::vector<std::map<std::string, std::string>> discoverDevices() = 0;

    virtual void setMessageCallback(MessageCallback callback) = 0;
    virtual void setStatusCallback(StatusCallback callback) = 0;

    virtual std::string getPrinterId() const = 0;
    virtual NetworkConfig getConfig() const = 0;
};

class PrinterNetworkFactory {
public:
    static std::unique_ptr<IPrinterNetwork> createNetwork(const PrintHostType hostType);
};
} // namespace Slic3r

#endif
