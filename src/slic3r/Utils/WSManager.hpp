#ifndef slic3r_WSManager_hpp_
#define slic3r_WSManager_hpp_

#include "WSTypes.hpp"
#include "Singleton.hpp"
#include <string>
#include <memory>
#include <map>
#include <mutex>

namespace Slic3r {

class WSClient;

class WSManager : public Singleton<WSManager> {
    friend class Singleton<WSManager>;
public:
    ~WSManager();
    
    bool addClient(const std::string& clientId, const std::string& url, 
                   std::shared_ptr<MessageHandler> msgHandler,
                   std::shared_ptr<ConnectionStatusHandler> statusHandler);
    
    
    void removeClient(const std::string& clientId);
    std::shared_ptr<WSClient> getClient(const std::string& clientId);
    std::string send(const std::string& clientId, const std::string& data, int timeoutMs = 5000);
    void close();

private:
    WSManager();
    mutable std::mutex mClientsMutex;
    std::map<std::string, std::shared_ptr<WSClient>> mClients;
};

} // namespace Slic3r

#endif 