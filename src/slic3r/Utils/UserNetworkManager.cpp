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

UserNetworkManager::~UserNetworkManager() { uninit(); }

void UserNetworkManager::init()
{
    std::lock_guard<std::mutex> lock(mInitMutex);
    if (mInitialized) {
        return;
    }
    IUserNetwork::init();
    loadUserInfo();

    mRunning       = true;
    mMonitorThread = std::thread([this]() { monitorLoop(); });
    mInitialized   = true;
}

void UserNetworkManager::uninit()
{
    std::lock_guard<std::mutex> lock(mInitMutex);
    if (!mInitialized) {
        return;
    }
    mRunning = false;
    if (mMonitorThread.joinable()) {
        mMonitorThread.join();
    }
    setNetwork(nullptr);
    mInitialized = false;
    IUserNetwork::uninit();
}
void UserNetworkManager::setIotUserInfo(const UserNetworkInfo& userInfo)
{
    UserNetworkInfo userNetworkInfo = userInfo;
    userNetworkInfo.connectedToIot  = false;
    setUserInfo(userNetworkInfo);
    saveUserInfo(userNetworkInfo);
}

UserNetworkInfo UserNetworkManager::getIotUserInfo() { return getUserInfo(); }

void UserNetworkManager::clearIotUserInfo()
{
    setUserInfo(UserNetworkInfo());
    setNetwork(nullptr);
    saveUserInfo(UserNetworkInfo());
}

PrinterNetworkResult<UserNetworkInfo> UserNetworkManager::getRtcToken()
{
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::SUCCESS, UserNetworkInfo());
    }
    return network->getRtcToken();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> UserNetworkManager::getUserBoundPrinters()
{
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, std::vector<PrinterNetworkInfo>());
    }

    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> result = network->getUserBoundPrinters();
    if (!result.isSuccess()) {
        LoginStatus loginStatus = parseLoginStatusByErrorCode(result.code);
        if (loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN || loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER ||
            loginStatus == LOGIN_STATUS_OFFLINE) {
            updateUserInfoLoginStatus(loginStatus, network->getUserNetworkInfo().userId);
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
    if (!network && mNetwork) {
        mNetwork->disconnectFromIot();
    }
    mNetwork = network;
}

bool UserNetworkManager::isValidToken(const UserNetworkInfo& userInfo)
{
    if (userInfo.loginStatus != LOGIN_STATUS_OFFLINE_INVALID_TOKEN && 
        userInfo.loginStatus != LOGIN_STATUS_OFFLINE_INVALID_USER &&
        userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED_REFRESH &&
        userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_NOT_EXPIRED_RELOGIN &&
        userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_REFRESH_FAILED_RELOGIN &&
        userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_REFRESH_FAILED_RETRY) {
        return true;
    }
    return false;
}

bool UserNetworkManager::needReLogin(const UserNetworkInfo& userInfo)
{
    return userInfo.userId.empty() || 
        userInfo.token.empty() || 
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_NOT_EXPIRED_RELOGIN ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_REFRESH_FAILED_RELOGIN;
}
void UserNetworkManager::monitorLoop()
{
    mLastLoopTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    while (mRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - mLastLoopTime).count();

        // execute main logic every 10 seconds
        if (elapsed < 10) {
            continue;
        }

        //return;

        mLastLoopTime = now;

        UserNetworkInfo               userInfo        = getUserInfo();
        std::shared_ptr<IUserNetwork> network         = getNetwork();

        // if user id is empty or token is empty or login status is invalid, need to re-login
        if (needReLogin(userInfo)) {
            if (network) {
                setNetwork(nullptr);
            }
            continue;
        }

        // check if user id changed
        bool  userChanged = false;
        if (network && network->getUserNetworkInfo().userId != userInfo.userId) {
            network->disconnectFromIot();
            network = nullptr;
            userChanged = true;
        }        
                
        // refresh token
        bool tokenRefreshed = refreshToken(userInfo, network);

        if(network && userInfo.connectedToIot && userInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
            // already connected to iot
            continue;
        }
        
        // connect to iot
        bool connectedToIot = false;
        if (!network || !userInfo.connectedToIot || userInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            if (isValidToken(userInfo)) {
                auto now                   = std::chrono::steady_clock::now();
                userInfo.connnectToIotTime = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
                if (network) {
                    network = nullptr;
                }
                network = NetworkFactory::createUserNetwork(userInfo);
                if (network) {
                    auto loginResult = network->connectToIot(userInfo);
                    if (loginResult.isSuccess() && loginResult.hasData()) {
                        userInfo.connectedToIot = true;
                        userInfo.loginStatus    = LOGIN_STATUS_LOGIN_SUCCESS;
                        connectedToIot = true;
                    } else {
                        userInfo.connectedToIot = false;
                        userInfo.loginStatus    = parseLoginStatusByErrorCode(loginResult.code);
                    }
                } else {
                    userInfo.connectedToIot = false;
                    userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
                }
            } else {
                userInfo.connectedToIot = false;
                wxLogMessage("User token validation failed, need to re-login or refresh token, user id: %s, login status: %d",
                             userInfo.userId.c_str(), userInfo.loginStatus);
            }
        }

        if (!userInfo.connectedToIot || userInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            if (network) {
                network = nullptr;
            }
        }

        if (!updateUserInfo(userInfo)) {
            // user id changed during connection, abandon this connection
            network = nullptr;
            wxLogMessage("User id changed during connection, abandon this connection, user id: %s", userInfo.userId.c_str());
        }

        setNetwork(network);

        if (!network || !userInfo.connectedToIot || userInfo.loginStatus != LOGIN_STATUS_LOGIN_SUCCESS) {
            continue;
        }

        if (tokenRefreshed  || (connectedToIot && !userChanged)) {
            // send event when token refreshed successfully
            // connected to iot case, send event when user login successfully
            // user changed case, send event when user login successfully, because user login event already sent by UserLoginView, don't send duplicate event
            for (int i = 0; i < 3; i++) { 
                if(!mRunning) {
                    break;
                }
                if (wxGetApp().mainframe && wxGetApp().mainframe->IsShown()) {
                    auto evt = new wxCommandEvent(EVT_USER_INFO_UPDATED);
                    wxQueueEvent(wxGetApp().mainframe, evt);
                    wxLogMessage("User token refreshed, send event to mainframe, user id: %s, user nickname: %s, login status: %d",
                                 userInfo.userId.c_str(), userInfo.nickname.c_str(), userInfo.loginStatus);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }


}

bool UserNetworkManager::refreshToken(UserNetworkInfo& userInfo, std::shared_ptr<IUserNetwork>& network)
{
    uint64_t nowTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    
    if (userInfo.refreshTokenExpireTime < nowTime) {
        network                 = nullptr;
        userInfo.connectedToIot = false;
        userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_TOKEN;
        return false;
    }
    
    if (userInfo.accessTokenExpireTime < nowTime) {
        userInfo.connectedToIot = false;
        userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED_REFRESH;
    } else {
        uint64_t tokenValidDiffTime = userInfo.accessTokenExpireTime - userInfo.lastTokenRefreshTime;
        uint64_t elapsedTokenTime   = nowTime - userInfo.lastTokenRefreshTime;
        if (elapsedTokenTime > tokenValidDiffTime / 2) {
            userInfo.connectedToIot = false;
            userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_TOKEN_NOT_EXPIRED_RELOGIN;
        }
    }

    if (userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED_REFRESH &&
        userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_NOT_EXPIRED_RELOGIN) {
        return false;
    }

    if (!network) {
        network = NetworkFactory::createUserNetwork(userInfo);
        if (!network) {
            userInfo.connectedToIot = false;
            userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
            wxLogMessage("User token refresh failed, network is null, user id: %s", userInfo.userId.c_str());
            return false;
        }
    }
    
    auto refreshResult = network->refreshToken(userInfo);
    if (refreshResult.isSuccess() && refreshResult.hasData()) {
        UserNetworkInfo refreshedUser   = refreshResult.data.value();
        userInfo.token                  = refreshedUser.token;
        userInfo.refreshToken           = refreshedUser.refreshToken;
        userInfo.accessTokenExpireTime  = refreshedUser.accessTokenExpireTime;
        userInfo.refreshTokenExpireTime = refreshedUser.refreshTokenExpireTime;
        userInfo.lastTokenRefreshTime   = nowTime;
        userInfo.loginStatus            = LOGIN_STATUS_LOGIN_SUCCESS;
        userInfo.connectedToIot         = true;
        wxLogMessage("User token refreshed successfully, user id: %s", userInfo.userId.c_str());
        return true;
    } 
    if (refreshResult.code == PrinterNetworkErrorCode::SERVER_UNAUTHORIZED ||
        refreshResult.code == PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD ||
        refreshResult.code == PrinterNetworkErrorCode::INVALID_TOKEN) {
        userInfo.loginStatus = LOGIN_STATUS_OFFLINE_TOKEN_REFRESH_FAILED_RELOGIN;
        wxLogMessage("User token refresh failed, need to re-login, user id: %s, error code: %d",
                     userInfo.userId.c_str(), static_cast<int>(refreshResult.code));
    } else {
        userInfo.loginStatus = LOGIN_STATUS_OFFLINE_TOKEN_REFRESH_FAILED_RETRY;
        wxLogMessage("User token refresh failed, need to retry, user id: %s, error code: %d",
                     userInfo.userId.c_str(), static_cast<int>(refreshResult.code));
    }
    return false;
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
            mUserInfo                = userInfo;
            mUserInfo.loginStatus    = LOGIN_STATUS_NOT_LOGIN;
            mUserInfo.connectedToIot = false;
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load user info: %s", e.what());
    }
}

} // namespace Slic3r
