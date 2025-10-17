#include "UserNetworkManager.hpp"
#include "PrinterCache.hpp"
#include "JsonUtils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r/Utils.hpp"
#include <wx/log.h>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

namespace Slic3r {

namespace fs = boost::filesystem;

UserNetworkManager::UserNetworkManager() {}

UserNetworkManager::~UserNetworkManager()
{
    uninit();
}

void UserNetworkManager::init()
{
    std::lock_guard<std::mutex> lock(mInitMutex);
    if (mInitialized) {
        return;
    }
    IUserNetwork::init();
    loadUserInfo();
    
    mRunning = true;
    mMonitorThread = std::thread([this]() { monitorLoop(); });
    mInitialized = true;
}

void UserNetworkManager::uninit()
{
    std::lock_guard<std::mutex> lock(mInitMutex);
    if (!mInitialized) {
        return;
    }
    IUserNetwork::uninit();
    mRunning = false;   
    if (mMonitorThread.joinable()) {
        mMonitorThread.join();
    }  
    setNetwork(nullptr);
    mInitialized = false;
}
void UserNetworkManager::checkInitialized()
{
    {
        std::lock_guard<std::mutex> lock(mInitMutex);
        if (mInitialized) {
            return;
        }
    }
    init();
}
void UserNetworkManager::setIotUserInfo(const UserNetworkInfo& userInfo)
{
    checkInitialized();
    UserNetworkInfo userNetworkInfo = userInfo;
    userNetworkInfo.connectedToIot = false;
    setUserInfo(userNetworkInfo);
    saveUserInfo(userNetworkInfo);
}

UserNetworkInfo UserNetworkManager::getIotUserInfo()
{
    checkInitialized();
    return getUserInfo();
}

void UserNetworkManager::clearIotUserInfo()
{
    checkInitialized();
    setUserInfo(UserNetworkInfo());
    setNetwork(nullptr);
    saveUserInfo(UserNetworkInfo());
}

PrinterNetworkResult<UserNetworkInfo> UserNetworkManager::getRtcToken()
{
    checkInitialized();
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::SUCCESS, UserNetworkInfo());
    }
    
    return network->getRtcToken();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> UserNetworkManager::getUserBoundPrinters()
{
    checkInitialized();
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, std::vector<PrinterNetworkInfo>());
    }

    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> result = network->getUserBoundPrinters();
    if (!result.isSuccess()) {
        PrinterNetworkErrorCode errorCode = result.code;
        switch (errorCode) {
        case PrinterNetworkErrorCode::INVALID_TOKEN: updateUserInfoLoginStatus(LOGIN_STATUS_OFFLINE_INVALID_TOKEN, mUserInfo.userId); break;
        case PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD:
            updateUserInfoLoginStatus(LOGIN_STATUS_OFFLINE_INVALID_USER, mUserInfo.userId);
            break;
        default: updateUserInfoLoginStatus(LOGIN_STATUS_OFFLINE_NETWORK_ERROR, mUserInfo.userId); break;
        }
    }
    return result;
}
UserNetworkInfo UserNetworkManager::getUserInfo() const
{
    std::lock_guard<std::mutex> lock(mUserInfoMutex);
    return mUserInfo;
}

void UserNetworkManager::setUserInfo(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mUserInfoMutex);
    mUserInfo = userInfo;
}

bool UserNetworkManager::updateUserInfo(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mUserInfoMutex);
    // if the user id is the same, update the user info
    if (mUserInfo.userId == userInfo.userId) {
        mUserInfo = userInfo;
        saveUserInfo(mUserInfo);
        return true;
    }
    // user id is different
    return false;
}

bool UserNetworkManager::updateUserInfoLoginStatus(const LoginStatus& loginStatus, const std::string& userId)
{
    std::lock_guard<std::mutex> lock(mUserInfoMutex);
    if (mUserInfo.userId == userId) {
        mUserInfo.loginStatus = loginStatus;
        saveUserInfo(mUserInfo);
        return true;
    }
    return false;
}
std::shared_ptr<IUserNetwork> UserNetworkManager::getNetwork() const
{
    std::lock_guard<std::mutex> lock(mUserInfoMutex);
    return mNetwork;
}

void UserNetworkManager::setNetwork(std::shared_ptr<IUserNetwork> network)
{
    std::lock_guard<std::mutex> lock(mUserInfoMutex);
    mNetwork = network;
}

void UserNetworkManager::monitorLoop()
{
    mLastLoopTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    
    while (mRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));    
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - mLastLoopTime).count();
        
        // execute main logic every 10 seconds
        if (elapsed < 10) {
            continue;
        }
        
        mLastLoopTime = now;

        UserNetworkInfo userInfo = getUserInfo();
        std::shared_ptr<IUserNetwork> network = getNetwork();
        LoginStatus lastLoginStatus = userInfo.loginStatus;
        
        if (userInfo.userId.empty() || userInfo.token.empty()) {
            if (network) {
                setNetwork(nullptr);
            }
            continue;
        }  
        
        bool userChanged = false;
        if (network && network->getUserNetworkInfo().userId != userInfo.userId) {
            // user changed
            network = nullptr;
            userChanged = true;
        }

        bool needRefreshToken = false;
        if (network && userInfo.connectedToIot && userInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
            auto now = std::chrono::steady_clock::now();
            // if token refresh time is half of the expire time, refresh token
            uint64_t nowTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
            if(nowTime - userInfo.lastTokenRefreshTime >= userInfo.accessTokenExpireTime / 2) {
                needRefreshToken = true;
            } else {
                continue;
            }
        }
        // connect to iot
        if (!userInfo.connectedToIot || userInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            auto now = std::chrono::steady_clock::now();
            userInfo.connnectToIotTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
            if(network) {
                network = nullptr;
            }
            network = NetworkFactory::createUserNetwork(userInfo);
            if (network) {
                auto loginResult = network->connectToIot(userInfo);
                if (!loginResult.hasData()) {
                    userInfo.connectedToIot = false;
                    userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
                } else if (loginResult.isSuccess()) {
                    userInfo.connectedToIot = true;
                    userInfo.loginStatus    = LOGIN_STATUS_LOGIN_SUCCESS;          
                } else {
                    UserNetworkInfo connectedUser = loginResult.data.value();
                    userInfo.connectedToIot       = connectedUser.connectedToIot;
                    userInfo.loginStatus          = connectedUser.loginStatus;
                }
            } else {
                userInfo.connectedToIot = false;
                userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
            }

        }
        
        // refresh token
        bool tokenRefreshed = false;
        if (network && userInfo.connectedToIot && userInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS && needRefreshToken) {          
            auto now = std::chrono::steady_clock::now();
            auto refreshResult = network->refreshToken(userInfo);
            if (refreshResult.isSuccess() && refreshResult.hasData()) {
                UserNetworkInfo refreshedUser = refreshResult.data.value();
                userInfo.token = refreshedUser.token;
                userInfo.refreshToken = refreshedUser.refreshToken;
                userInfo.accessTokenExpireTime = refreshedUser.accessTokenExpireTime;
                userInfo.refreshTokenExpireTime = refreshedUser.refreshTokenExpireTime;
                userInfo.lastTokenRefreshTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
                tokenRefreshed = true;
                wxLogMessage("User token refreshed successfully");
            }                   
        }    

        if(!userInfo.connectedToIot || userInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            if(network) {
                network = nullptr;
            }
        }

        if (!updateUserInfo(userInfo)) {
            // user id changed during connection, abandon this connection
            network = nullptr;
            wxLogMessage("User id changed during connection, abandon this connection, user id: %s", userInfo.userId.c_str());

        }

        setNetwork(network);

        if(!network || !userInfo.connectedToIot || userInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            continue;
        }

        if(tokenRefreshed || !userChanged) {
            // 1 refresh token 
            // 2 connected to iot and login status changed(first login in userLoginView don't need to send event)
            // send event to mainframe
            // user changed, don't send event
            auto evt = new wxCommandEvent(EVT_USER_INFO_UPDATED);
            wxQueueEvent(wxGetApp().mainframe, evt);       
            wxLogMessage("User info updated, send event to mainframe, user id: %s, user nickname: %s, login status: %d", userInfo.userId.c_str(), userInfo.nickname.c_str(), userInfo.loginStatus);
        }
    }
}

void UserNetworkManager::saveUserInfo(const UserNetworkInfo& userInfo)
{
    try {
        fs::path path = fs::path(Slic3r::data_dir()) / "user" / "user_info.json";
        
        if (!fs::exists(path.parent_path())) {
            fs::create_directories(path.parent_path());
        }
        
        std::ofstream file(path.string());
        if (file.is_open()) {
            file << convertUserNetworkInfoToJson(userInfo).dump(4);
            file.close();
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to save user info: %s", e.what());
    }
}

void UserNetworkManager::loadUserInfo()
{
    try {
        fs::path path = fs::path(Slic3r::data_dir()) / "user" / "user_info.json";
        
        if (!fs::exists(path)) {
            return;
        }
        
        std::ifstream file(path.string());
        if (!file.is_open()) {
            return;
        }
        
        nlohmann::json json;
        file >> json;
        file.close();
        
        UserNetworkInfo userInfo = convertJsonToUserNetworkInfo(json);
        
        if (!userInfo.userId.empty()) {
            mUserInfo = userInfo;
            mUserInfo.loginStatus = LOGIN_STATUS_NOT_LOGIN;
            mUserInfo.connectedToIot = false;
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load user info: %s", e.what());
    }
}

} // namespace Slic3r
