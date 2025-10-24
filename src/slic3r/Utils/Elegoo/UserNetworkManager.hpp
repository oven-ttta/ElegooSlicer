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
    
    UserNetworkInfo getUserInfo() const;
    // only can be called by login or logout
    void setUserInfo(const UserNetworkInfo& userInfo);
    // for frontend click avatar, check user need re-login
    PrinterNetworkResult<bool> checkUserNeedReLogin();
    PrinterNetworkResult<UserNetworkInfo> getRtcToken();
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> getUserBoundPrinters();
    
    bool refreshToken(const UserNetworkInfo& userInfo);

private:
    UserNetworkManager();
    ~UserNetworkManager();
    
    void monitorLoop();
    bool refreshToken(UserNetworkInfo& userInfo, std::shared_ptr<IUserNetwork>& network);

    std::shared_ptr<IUserNetwork> getNetwork() const;
    void setNetwork(std::shared_ptr<IUserNetwork> network);
        
    void saveUserInfo(const UserNetworkInfo& userInfo);
    void loadUserInfo();
    bool updateUserInfo(const UserNetworkInfo& userInfo);
    bool updateUserInfoLoginStatus(const LoginStatus& loginStatus, const std::string& userId);
    bool needReLogin(const UserNetworkInfo& userInfo);

    std::string getLoginErrorMessage(const UserNetworkInfo& userInfo);
    void notifyUserInfoUpdated();
private:
    mutable std::mutex mNetworkMutex;
    UserNetworkInfo mUserInfo;
    std::shared_ptr<IUserNetwork> mNetwork;
    
    mutable std::mutex mInitMutex;
    std::atomic<bool> mRunning{false};
    std::thread mMonitorThread;
    bool mInitialized{false};
    
    std::chrono::steady_clock::time_point mLastLoopTime;

    std::mutex mMonitorMutex;
};

} // namespace Slic3r

#endif
