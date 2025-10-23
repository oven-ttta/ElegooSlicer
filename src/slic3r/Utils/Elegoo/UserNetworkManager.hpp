#ifndef slic3r_UserNetworkManager_hpp_
#define slic3r_UserNetworkManager_hpp_

#include "Singleton.hpp"
#include "PrinterNetwork.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

namespace Slic3r {

class UserNetworkManager : public Singleton<UserNetworkManager> {
    friend class Singleton<UserNetworkManager>;

public:
    UserNetworkManager(const UserNetworkManager&) = delete;
    UserNetworkManager& operator=(const UserNetworkManager&) = delete;

    void init();
    void uninit();
    
    void setIotUserInfo(const UserNetworkInfo& userInfo);
    UserNetworkInfo getIotUserInfo();
    void clearIotUserInfo();
    
    PrinterNetworkResult<UserNetworkInfo> getRtcToken();
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> getUserBoundPrinters();
    
private:
    UserNetworkManager();
    ~UserNetworkManager();
    
    void monitorLoop();
    bool refreshToken(UserNetworkInfo& userInfo, std::shared_ptr<IUserNetwork>& network);
    bool isValidToken(const UserNetworkInfo& userInfo);
    bool needReLogin(const UserNetworkInfo& userInfo);

    UserNetworkInfo getUserInfo() const;
    void setUserInfo(const UserNetworkInfo& userInfo);
    std::shared_ptr<IUserNetwork> getNetwork() const;
    void setNetwork(std::shared_ptr<IUserNetwork> network);
        
    void saveUserInfo(const UserNetworkInfo& userInfo);
    void loadUserInfo();
    bool updateUserInfo(const UserNetworkInfo& userInfo);
    bool updateUserInfoLoginStatus(const LoginStatus& loginStatus, const std::string& userId);

private:
    mutable std::mutex mUserInfoMutex;
    UserNetworkInfo mUserInfo;
    std::shared_ptr<IUserNetwork> mNetwork;
    
    mutable std::mutex mInitMutex;
    std::atomic<bool> mRunning{false};
    std::thread mMonitorThread;
    bool mInitialized{false};
    
    std::chrono::steady_clock::time_point mLastLoopTime;
};

} // namespace Slic3r

#endif
