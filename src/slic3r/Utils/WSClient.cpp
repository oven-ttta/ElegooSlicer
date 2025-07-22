#include "WSClient.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <regex>
#include <random>

namespace beast = boost::beast;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace Slic3r {

class WSClient::Impl {
public:
    std::string host;
    std::string port;
    std::string path;
    
    asio::io_context ioc;
    beast::websocket::stream<tcp::socket> ws;
    std::unique_ptr<asio::io_context::work> work;
    
    Impl() : ws(ioc) {
        work = std::make_unique<asio::io_context::work>(ioc);
    }
    
    ~Impl() {
        work.reset();
        if (ioc.stopped()) {
            ioc.run();
        }
    }
    
    bool parseURL(const std::string& url) {
        std::regex urlRegex(R"(ws://([^:/]+):([0-9]+)(/.*)?)");
        std::smatch match;
        if (std::regex_match(url, match, urlRegex)) {
            host = match[1].str();
            port = match[2].str();
            path = match[3].matched ? match[3].str() : "/";
            return true;
        }
        return false;
    }
    
    bool connect(const std::string& url, int timeoutMs) {
        try {
            if (!parseURL(url)) {
                return false;
            }
            
            tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, port);
            
            asio::connect(ws.next_layer(), results.begin(), results.end());
            ws.handshake(host, path);
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "WebSocket connection failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void disconnect() {
        try {
            if (ws.is_open()) {
                ws.close(beast::websocket::close_code::normal);
            }
        } catch (const std::exception& e) {
            std::cerr << "WebSocket disconnect error: " << e.what() << std::endl;
        }
    }
    
    bool isConnected() const {
        return ws.is_open();
    }
    
    bool send(const std::string& data) {
        try {
            ws.write(asio::buffer(data));
            return true;
        } catch (const std::exception& e) {
            std::cerr << "WebSocket send error: " << e.what() << std::endl;
            return false;
        }
    }
    
    std::string receive() {
        try {
            beast::flat_buffer buffer;
            ws.read(buffer);
            return beast::buffers_to_string(buffer.data());
        } catch (const std::exception& e) {
            std::cerr << "WebSocket receive error: " << e.what() << std::endl;
            return "";
        }
    }
};

WSClient::WSClient(const std::string& id, const std::string& url, 
                   std::shared_ptr<MessageHandler> msgHandler,
                   std::shared_ptr<ConnectionStatusHandler> statusHandler)
    : mId(id)
    , mUrl(url)
    , mMessageHandler(msgHandler)
    , mStatusHandler(statusHandler)
    , mStatus(WSStatus::Disconnected)
    , mShouldStop(false)
    , mActiveThreads(0)
    , mImpl(std::make_shared<Impl>())
{
}

WSClient::~WSClient() {
    disconnect();
}

bool WSClient::connect(int timeoutMs) {
    if (getStatus() != WSStatus::Disconnected) {
        return false;
    }
    
    setStatus(WSStatus::Connecting);
    notifyStatus(WSStatus::Connecting);
    
    try {
        if (mImpl->connect(mUrl, timeoutMs)) {
            setStatus(WSStatus::Connected);
            mShouldStop = false;
            mActiveThreads = 3;
            
            mReadThread = std::thread(&WSClient::readMessages, this);
            mWriteThread = std::thread(&WSClient::writeMessages, this);
            mReportThread = std::thread(&WSClient::handleReports, this);
            
            notifyStatus(WSStatus::Connected);
            return true;
        } else {
            setStatus(WSStatus::Error);
            notifyStatus(WSStatus::Error, "Connection failed");
            return false;
        }
    } catch (const std::exception& e) {
        setStatus(WSStatus::Error);
        notifyStatus(WSStatus::Error, e.what());
        return false;
    }
}

void WSClient::disconnect() {
    if (mShouldStop.exchange(true)) {
        return; // Already disconnecting
    }
    
    setStatus(WSStatus::Disconnected);
    
    // Wake up threads
    mSendCV.notify_all();
    mReportCV.notify_all();
    
    // Wait for threads to finish
    if (mReadThread.joinable()) mReadThread.join();
    if (mWriteThread.joinable()) mWriteThread.join();
    if (mReportThread.joinable()) mReportThread.join();
    
    mImpl->disconnect();
    
    // Clear responses
    {
        std::lock_guard<std::mutex> lock(mResponseMutex);
        for (auto& pair : mResponses) {
            pair.second.set_value(WSResponse("", "Client disconnected"));
        }
        mResponses.clear();
    }
    
    notifyStatus(WSStatus::Disconnected);
}

bool WSClient::isConnected() const {
    return getStatus() == WSStatus::Connected && mImpl->isConnected();
}

WSStatus WSClient::getStatus() const {
    return mStatus.load();
}

std::string WSClient::send(const std::string& data, int timeoutMs) {
    if (getStatus() != WSStatus::Connected) {
        throw std::runtime_error("Client not connected");
    }
    
    if (!mMessageHandler) {
        throw std::runtime_error("Message handler not set");
    }
    
    std::string requestId = mMessageHandler->getRequestID(data);
    if (requestId.empty()) {
        throw std::runtime_error("Missing request ID");
    }
    
    std::promise<WSResponse> promise;
    auto future = promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(mResponseMutex);
        mResponses[requestId] = std::move(promise);
    }
    
    // Add to send queue
    {
        std::lock_guard<std::mutex> lock(mSendMutex);
        mSendQueue.emplace(1, data); // 1 = TextMessage
    }
    mSendCV.notify_one();
    
    try {
        if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout) {
            {
                std::lock_guard<std::mutex> lock(mResponseMutex);
                mResponses.erase(requestId);
            }
            throw std::runtime_error("Request timeout");
        }
        
        auto response = future.get();
        if (!response.error.empty()) {
            throw std::runtime_error(response.error);
        }
        return response.data;
        
    } catch (const std::exception& e) {
        {
            std::lock_guard<std::mutex> lock(mResponseMutex);
            mResponses.erase(requestId);
        }
        throw;
    }
}

void WSClient::wait() {
    while (mActiveThreads.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void WSClient::readMessages() {
    while (!mShouldStop && getStatus() == WSStatus::Connected) {
        try {
            std::string message = mImpl->receive();
            if (!message.empty() && mMessageHandler) {
                if (mMessageHandler->isReport(message)) {
                    // Add to report queue
                    {
                        std::lock_guard<std::mutex> lock(mReportMutex);
                        mReportQueue.push(message);
                    }
                    mReportCV.notify_one();
                } else if (mMessageHandler->isResponse(message)) {
                    std::string requestId = mMessageHandler->getRequestID(message);
                    if (!requestId.empty()) {
                        std::lock_guard<std::mutex> lock(mResponseMutex);
                        auto it = mResponses.find(requestId);
                        if (it != mResponses.end()) {
                            it->second.set_value(WSResponse(message));
                            mResponses.erase(it);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            if (getStatus() == WSStatus::Connected) {
                setStatus(WSStatus::Error);
                notifyStatus(WSStatus::Error, e.what());
                break;
            }
        }
    }
    mActiveThreads--;
}

void WSClient::writeMessages() {
    while (!mShouldStop) {
        WSMessage msg;
        {
            std::unique_lock<std::mutex> lock(mSendMutex);
            mSendCV.wait(lock, [this] { return !mSendQueue.empty() || mShouldStop; });
            
            if (mShouldStop) break;
            
            msg = mSendQueue.front();
            mSendQueue.pop();
        }
        
        if (getStatus() == WSStatus::Connected) {
            mImpl->send(msg.data);
        }
    }
    mActiveThreads--;
}

void WSClient::handleReports() {
    while (!mShouldStop) {
        std::string report;
        {
            std::unique_lock<std::mutex> lock(mReportMutex);
            mReportCV.wait(lock, [this] { return !mReportQueue.empty() || mShouldStop; });
            
            if (mShouldStop) break;
            
            report = mReportQueue.front();
            mReportQueue.pop();
        }
        
        if (mMessageHandler) {
            mMessageHandler->handleReport(report);
        }
    }
    mActiveThreads--;
}

void WSClient::setStatus(WSStatus status) {
    mStatus.store(status);
}

void WSClient::notifyStatus(WSStatus status, const std::string& error) {
    if (mStatusHandler) {
        mStatusHandler->onStatusChange(mId, status, error);
    }
}

} // namespace Slic3r