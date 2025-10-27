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

#define CHECK_INITIALIZED(returnVal) \
    do { \
        std::lock_guard<std::mutex> lock(mInitMutex); \
        if(!mInitialized) { \
            return returnVal; \
        } \
    } while(0)


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

PrinterNetworkResult<UserNetworkInfo> UserNetworkManager::getRtcToken()
{
    CHECK_INITIALIZED(PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::NOT_INITIALIZED, UserNetworkInfo()));
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::SUCCESS, UserNetworkInfo());
    }
    return network->getRtcToken();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> UserNetworkManager::getUserBoundPrinters()
{
    CHECK_INITIALIZED(PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::NOT_INITIALIZED, std::vector<PrinterNetworkInfo>()));
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, std::vector<PrinterNetworkInfo>());
    }

    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> result = network->getUserBoundPrinters();
    if (!result.isSuccess()) {
        LoginStatus loginStatus = parseLoginStatusByErrorCode(result.code);
        if (loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED || loginStatus == LOGIN_STATUS_OFFLINE) {
            updateUserInfoLoginStatus(loginStatus, network->getUserNetworkInfo().userId);
        }
    }
    return result;
}


UserNetworkInfo UserNetworkManager::getUserInfo() const
{
    CHECK_INITIALIZED(UserNetworkInfo());
    std::lock_guard<std::mutex> lock(mNetworkMutex);
    return mUserInfo;
}

void UserNetworkManager::setUserInfo(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mNetworkMutex);
    UserNetworkInfo userNetworkInfo = userInfo;
    if(userNetworkInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
        // connectedToIot set to true for frontend login success display, but actually not connected to iot yet, still need to connect iot successfully
        userNetworkInfo.connectedToIot = true;
    } else {
        userNetworkInfo.connectedToIot = false;
    }
    mUserInfo = userNetworkInfo;
    

    // set network to nullptr and monitorLoop will reset connect to iot
    if (mNetwork) {
        mNetwork->disconnectFromIot();
        mNetwork = nullptr;
    }
    
    notifyUserInfoUpdated();
    saveUserInfo(userNetworkInfo);
}

std::shared_ptr<IUserNetwork> UserNetworkManager::getNetwork() const
{
    std::lock_guard<std::mutex> lock(mNetworkMutex);
    return mNetwork;
}

void UserNetworkManager::setNetwork(std::shared_ptr<IUserNetwork> network)
{
    std::lock_guard<std::mutex> lock(mNetworkMutex);
    if (!network && mNetwork) {
        mNetwork->disconnectFromIot();
    }
    mNetwork = network;
}
bool UserNetworkManager::updateUserInfo(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mNetworkMutex);
    // if the user id is the same, update the user info
    if (mUserInfo.userId == userInfo.userId) {
        bool needNotify = false;
        if (mUserInfo.loginStatus != userInfo.loginStatus) {
            needNotify = true;
            wxLogMessage("User login status updated, user id: %s, login status: %d",
                         userInfo.userId.c_str(), userInfo.loginStatus);
        }
        if (mUserInfo.connectedToIot != userInfo.connectedToIot) {
            needNotify = true;
            wxLogMessage("User connected to iot updated, user id: %s, connected to iot: %d",
                         userInfo.userId.c_str(), userInfo.connectedToIot);
        }
        if (mUserInfo.token != userInfo.token) {
            needNotify = true;
            wxLogMessage("User token updated, user id: %s, token: %s",
                         userInfo.userId.c_str(), userInfo.token.c_str());
        }
        if (mUserInfo.avatar != userInfo.avatar) {
            needNotify = true;
            wxLogMessage("User avatar updated, user id: %s, avatar: %s",
                         userInfo.userId.c_str(), userInfo.avatar.c_str());
        }
        if (mUserInfo.nickname != userInfo.nickname) {
            needNotify = true;
            wxLogMessage("User nickname updated, user id: %s, nickname: %s",
                         userInfo.userId.c_str(), userInfo.nickname.c_str());
        }
        mUserInfo = userInfo;
        mUserInfo.loginErrorMessage = getLoginErrorMessage(userInfo);
        if (needNotify) {
            notifyUserInfoUpdated();
        }
        saveUserInfo(mUserInfo);
        return true;
    }
    // user id is different
    return false;
}

bool UserNetworkManager::updateUserInfoLoginStatus(const LoginStatus& loginStatus, const std::string& userId)
{
    std::lock_guard<std::mutex> lock(mNetworkMutex);
    if(mUserInfo.userId != userId) {
        return false;
    }
    if(mUserInfo.loginStatus != loginStatus) {
        mUserInfo.loginStatus = loginStatus;
        mUserInfo.loginErrorMessage = getLoginErrorMessage(mUserInfo);
        notifyUserInfoUpdated();
        saveUserInfo(mUserInfo);       
    }
    return true;
}
void UserNetworkManager::notifyUserInfoUpdated()
{
    if (wxGetApp().mainframe && wxGetApp().mainframe->is_loaded()) {
        auto evt = new wxCommandEvent(EVT_USER_INFO_UPDATED);
        wxQueueEvent(wxGetApp().mainframe, evt);
        wxLogMessage("User info updated, send event to mainframe, user id: %s, login status: %d",
                     mUserInfo.userId.c_str(), mUserInfo.loginStatus);
    } else {
        wxLogMessage("Mainframe is not loaded, skip sending event, user id: %s, login status: %d",
                     mUserInfo.userId.c_str(), mUserInfo.loginStatus);
    }
}

PrinterNetworkResult<bool> UserNetworkManager::checkUserNeedReLogin()
{
    CHECK_INITIALIZED(PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_INITIALIZED, false));
    UserNetworkInfo currentUserInfo = getUserInfo();
    if (needReLogin(currentUserInfo)) {
        //need re-login
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, true);
    } else if(currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE || 
        currentUserInfo.loginStatus == LOGIN_STATUS_OTHER_NETWORK_ERROR ||
        currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED) {
        //network error or token expired, don't need to re-login, return network error message
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NETWORK_ERROR, false);
    } 
    //don't need to re-login
    return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::SUCCESS, false);
}
std::string UserNetworkManager::getLoginErrorMessage(const UserNetworkInfo& userInfo)
{
    if(userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN) {
        return getErrorMessage(PrinterNetworkErrorCode::INVALID_TOKEN);
    } else if(userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER) {
        return getErrorMessage(PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD);
    } else if(userInfo.loginStatus == LOGIN_STATUS_OFFLINE) {
        return getErrorMessage(PrinterNetworkErrorCode::NETWORK_ERROR);
    }
    return "";
}

bool UserNetworkManager::needReLogin(const UserNetworkInfo& userInfo)
{
    return userInfo.userId.empty() || 
        userInfo.token.empty() || 
        userInfo.loginStatus == LOGIN_STATUS_NO_USER ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER;
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
        std::lock_guard<std::mutex> lock(mMonitorMutex);
        mLastLoopTime = now;
        UserNetworkInfo               userInfo        = getUserInfo();
        std::shared_ptr<IUserNetwork> network         = getNetwork();
        LoginStatus lastLoginStatus = userInfo.loginStatus;

        // if user id is empty or token is empty or login status is invalid, need to re-login
        if (needReLogin(userInfo)) {
            if(network) {
                setNetwork(nullptr);
                wxLogMessage("User info or token invalid, need to re-login, user id: %s, login status: %d",
                             userInfo.userId.c_str(), userInfo.loginStatus);
            }
            continue;
        }

        // check if user id changed
        if (network && network->getUserNetworkInfo().userId != userInfo.userId) {
            setNetwork(nullptr);
            wxLogMessage("User id changed, need to re-login, user id: %s, login status: %d",
                         userInfo.userId.c_str(), userInfo.loginStatus);
            continue;
        }        
                
        // refresh token
        bool tokenRefreshed = refreshToken(userInfo, network);
        if(tokenRefreshed) {
            // if refresh token success, is already connected to iot
            uint64_t nowTime = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
            userInfo.lastTokenRefreshTime   = nowTime;
            userInfo.loginStatus            = LOGIN_STATUS_LOGIN_SUCCESS;
            userInfo.connectedToIot         = true;

            if(!updateUserInfo(userInfo)) {
                network = nullptr;
                wxLogMessage("User id changed during token refresh, abandon this refresh, user id: %s", userInfo.userId.c_str());
            } 
            setNetwork(network);           
            continue;
        }

        // if token is invalid, need to re-login
        // if token is expired, need to refresh token
        if (userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN || 
            userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER ||
            userInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED) {
            if (lastLoginStatus != userInfo.loginStatus) {
                setNetwork(nullptr);
                updateUserInfoLoginStatus(userInfo.loginStatus, userInfo.userId);
            }
            continue;
        }

        //  if already connected to iot, skip
        if (network && userInfo.connectedToIot && userInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
            continue;
        }
 
        if (!network) {
            // create network
            network = NetworkFactory::createUserNetwork(userInfo);
            if (!network) {
                userInfo.connectedToIot = false;
                userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
                if (lastLoginStatus != userInfo.loginStatus) {
                    setNetwork(nullptr);
                    updateUserInfoLoginStatus(userInfo.loginStatus, userInfo.userId);
                }
                continue;
            }
        }
         // connect to iot
        auto loginResult = network->connectToIot(userInfo);
        if (loginResult.isSuccess() && loginResult.hasData()) {
            UserNetworkInfo loginUserInfo = loginResult.data.value();
            userInfo.connectedToIot       = true;
            userInfo.loginStatus          = LOGIN_STATUS_LOGIN_SUCCESS;
            
            if (!loginUserInfo.avatar.empty() && loginUserInfo.avatar != userInfo.avatar) {
                userInfo.avatar = loginUserInfo.avatar;
            }
            if (!loginUserInfo.nickname.empty() && loginUserInfo.nickname != userInfo.nickname) {
                userInfo.nickname = loginUserInfo.nickname;
            }
            if (!loginUserInfo.email.empty() && loginUserInfo.email != userInfo.email) {
                userInfo.email = loginUserInfo.email;
            }
            
            if (updateUserInfo(userInfo)) {
                setNetwork(network);
            } else {
                // user id changed during connection, abandon this connection
                network = nullptr;
                setNetwork(nullptr);
                wxLogMessage("User id changed during connection, abandon this connection, user id: %s", userInfo.userId.c_str());
            }
        } else {
            userInfo.connectedToIot = false;
            userInfo.loginStatus    = parseLoginStatusByErrorCode(loginResult.code);
            
            if (lastLoginStatus != userInfo.loginStatus) {
                updateUserInfoLoginStatus(userInfo.loginStatus, userInfo.userId);
            }
        }
    }
}

UserNetworkInfo UserNetworkManager::refreshToken(const UserNetworkInfo& userInfo)
{
    CHECK_INITIALIZED(UserNetworkInfo());

    lock_guard<std::mutex> lock(mMonitorMutex);

    wxLogMessage("UserNetworkManager::refreshToken start refresh token, user id: %s, user token: %s, user refresh token: %s, user login "
                 "refresh time: %d, user access token expire time: %d, user refresh token expire time: %d",
                 userInfo.userId.c_str(), userInfo.token.c_str(), userInfo.refreshToken.c_str(), userInfo.lastTokenRefreshTime,
                 userInfo.accessTokenExpireTime, userInfo.refreshTokenExpireTime);

    UserNetworkInfo currentUserInfo = getUserInfo();
    std::shared_ptr<IUserNetwork> network = getNetwork();            

    // if user login status is invalid token or invalid user, skip refresh token
    if(currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN || 
        currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER) {
        wxLogMessage("UserNetworkManager::refreshToken user login status is invalid token or invalid user, skip refresh token, user id: %s", userInfo.userId.c_str());
        return UserNetworkInfo();
    }
    if(userInfo.userId != currentUserInfo.userId) {
        if(currentUserInfo.userId.empty()) {
            wxLogMessage("UserNetworkManager::refreshToken user id changed, current user id is empty, return empty user info, user id: %s", userInfo.userId.c_str());
            return UserNetworkInfo();
        }
        if(currentUserInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
            wxLogMessage("UserNetworkManager::refreshToken user id changed, current user id is not empty and user already logged in, return current user info, user id: %s", userInfo.userId.c_str());
            return currentUserInfo;
        }
    }
    if(userInfo.userId.empty() && currentUserInfo.userId.empty()) {
        wxLogMessage("UserNetworkManager::refreshToken user id is empty and current user id is empty, return empty user info, user id: %s", userInfo.userId.c_str());
        return UserNetworkInfo();
    }

    if(userInfo.userId.empty() && !currentUserInfo.userId.empty() && currentUserInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
        wxLogMessage("UserNetworkManager::refreshToken user id is empty and current user id is not empty and user already logged in, return current user info");
        return currentUserInfo;
    }

    // if already refreshed token, skip refresh token
    if(currentUserInfo.lastTokenRefreshTime > userInfo.lastTokenRefreshTime && currentUserInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
        wxLogMessage("already refreshed token, skip refresh token, user id: %s", userInfo.userId.c_str());
        return currentUserInfo;
    }

    if(network && currentUserInfo.userId != network->getUserNetworkInfo().userId) {
        // user id changed
        network = nullptr;
        wxLogMessage("UserNetworkManager::refreshToken user id changed, set network to nullptr, user id: %s", userInfo.userId.c_str());
    }

    if(refreshToken(currentUserInfo, network)) {
        wxLogMessage("UserNetworkManager::refreshToken refresh token success, user id: %s", userInfo.userId.c_str());
        updateUserInfo(currentUserInfo);
        setNetwork(network);
    } else if(currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN) {
        updateUserInfo(currentUserInfo);
        setNetwork(nullptr);
        wxLogMessage("UserNetworkManager::refreshToken refresh token failed, need to re-login, user id: %s", userInfo.userId.c_str());
        return UserNetworkInfo();
    }
    wxLogMessage("UserNetworkManager::refreshToken refresh token failed, user id: %s, login status: %d",
                 userInfo.userId.c_str(), userInfo.loginStatus);
    return currentUserInfo;
}


bool UserNetworkManager::refreshToken(UserNetworkInfo& userInfo, std::shared_ptr<IUserNetwork>& network)
{
    uint64_t nowTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    
    // token expired
    if (userInfo.refreshTokenExpireTime < nowTime && userInfo.accessTokenExpireTime < nowTime) {
        network                 = nullptr;
        userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_TOKEN;
        return false;
    }
    
    bool needRefresh = userInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED;
    if (userInfo.accessTokenExpireTime < nowTime) {
        needRefresh = true;
        userInfo.loginStatus   = LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED;
    } else {
        uint64_t tokenValidDiffTime = userInfo.accessTokenExpireTime - userInfo.lastTokenRefreshTime;
        uint64_t elapsedTokenTime   = nowTime - userInfo.lastTokenRefreshTime;
        if (elapsedTokenTime > tokenValidDiffTime / 2) {
            needRefresh = true;
        }
    }

    if (!needRefresh) {
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
        wxLogMessage("User token refreshed successfully, user id: %s", userInfo.userId.c_str());
        return true;
    } 
    if (refreshResult.code == PrinterNetworkErrorCode::SERVER_UNAUTHORIZED) {
        userInfo.loginStatus = LOGIN_STATUS_OFFLINE_INVALID_TOKEN;
        wxLogMessage("User token refresh failed, need to re-login, user id: %s, error code: %d",
                     userInfo.userId.c_str(), static_cast<int>(refreshResult.code));
    } 
    wxLogMessage("User token refresh failed, user id: %s, status: %d, error code: %d, error message: %s",
                 userInfo.userId.c_str(), userInfo.loginStatus, static_cast<int>(refreshResult.code), refreshResult.message.c_str());
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
            std::lock_guard<std::mutex> lock(mNetworkMutex);
            userInfo.loginStatus    = LOGIN_STATUS_NOT_LOGIN;
            userInfo.connectedToIot = false;
            mUserInfo               = userInfo;
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load user info: %s", e.what());
    }
}

} // namespace Slic3r
