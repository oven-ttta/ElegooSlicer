#ifndef slic3r_WSClient_hpp_
#define slic3r_WSClient_hpp_

#include "WSTypes.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <future>
#include <queue>
#include <condition_variable>
#include <memory>

namespace Slic3r {

class WSClient {
public:
    WSClient(const std::string& id, const std::string& url, 
             std::shared_ptr<MessageHandler> msgHandler,
             std::shared_ptr<ConnectionStatusHandler> statusHandler);
    ~WSClient();
    
    bool connect(int timeoutMs = 5000);
    void disconnect();
    bool isConnected() const;
    WSStatus getStatus() const;
    
    std::string send(const std::string& data, int timeoutMs = 5000);
    
    std::string getId() const { return mId; }
    std::string getUrl() const { return mUrl; }
    
    void wait();

private:
    class Impl;
    std::shared_ptr<Impl> mImpl;
    
    std::string mId;
    std::string mUrl;
    std::shared_ptr<MessageHandler> mMessageHandler;
    std::shared_ptr<ConnectionStatusHandler> mStatusHandler;
    
    std::atomic<WSStatus> mStatus;
    std::atomic<bool> mShouldStop;
    
    std::thread mReadThread;
    std::thread mWriteThread;
    std::thread mReportThread;
    std::atomic<int> mActiveThreads;
    
    std::mutex mSendMutex;
    std::queue<WSMessage> mSendQueue;
    std::condition_variable mSendCV;
    
    std::mutex mReportMutex;
    std::queue<std::string> mReportQueue;
    std::condition_variable mReportCV;
    
    std::mutex mResponseMutex;
    std::map<std::string, std::promise<WSResponse>> mResponses;
    
    void readMessages();
    void writeMessages();
    void handleReports();
    void setStatus(WSStatus status);
    void notifyStatus(WSStatus status, const std::string& error = "");
};

} // namespace Slic3r

#endif 