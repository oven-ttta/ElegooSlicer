#ifndef slic3r_WSTypes_hpp_
#define slic3r_WSTypes_hpp_

#include <string>
#include <functional>
#include <memory>
#include <atomic>

namespace Slic3r {

enum class WSStatus { Disconnected = 0, Connecting = 1, Connected = 2, Error = 3 };

struct WSMessage {
    int messageType;
    std::string data;
    WSMessage() : messageType(1) {} 
    WSMessage(int type, const std::string& msgData) : messageType(type), data(msgData) {}
};

struct WSResponse {
    std::string data;
    std::string error;
    WSResponse() {}
    WSResponse(const std::string& msgData, const std::string& err = "") : data(msgData), error(err) {}
};


class MessageHandler {
public:
    virtual ~MessageHandler() = default;
    virtual std::string getRequestID(const std::string& data) = 0;
    virtual bool isResponse(const std::string& data) = 0;
    virtual bool isReport(const std::string& data) = 0;
    virtual void handleReport(const std::string& data) = 0;
};

class ConnectionStatusHandler {
public:
    virtual ~ConnectionStatusHandler() = default;
    virtual void onStatusChange(const std::string& clientId, WSStatus status, const std::string& error) = 0;
};

} // namespace Slic3r

#endif 