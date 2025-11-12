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
#include <boost/nowide/fstream.hpp>

#define CHECK_INITIALIZED(returnVal) \
    std::lock_guard<std::recursive_mutex> __initLock(mInitMutex); \
    if(!mIsInitialized.load()) { \
        return returnVal; \
    }
namespace Slic3r {

namespace fs = boost::filesystem;

UserNetworkManager::UserNetworkManager() {}

UserNetworkManager::~UserNetworkManager() { uninit(); }

void UserNetworkManager::init()
{
    std::lock_guard<std::recursive_mutex> lock(mInitMutex);
    if (mIsInitialized.load()) {
        return;
    }
    loadUserInfo();

    mRunning.store(true);
    mMonitorThread = std::thread([this]() { monitorUserNetwork(); });
    mIsInitialized.store(true);
}

void UserNetworkManager::uninit()
{
    {
        std::lock_guard<std::recursive_mutex> lock(mInitMutex);
        if (!mIsInitialized.load()) {
            return;
        }
    }
    
    mRunning.store(false);
    if (mMonitorThread.joinable()) {
        mMonitorThread.join();
    }
    
    {
        std::lock_guard<std::recursive_mutex> lock(mInitMutex);
        setNetwork(nullptr);
        mIsInitialized.store(false);
    } 
}

PrinterNetworkResult<UserNetworkInfo> UserNetworkManager::getRtcToken()
{
    CHECK_INITIALIZED(PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::NOT_INITIALIZED, UserNetworkInfo()));
    
    std::shared_ptr<IUserNetwork> network = getNetwork();
    // Record request context before sending request 
    UserNetworkInfo requestUserInfo = network->getUserNetworkInfo();
    if (!network) {
        if(requestUserInfo.userId.empty()) {
            return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD, UserNetworkInfo());
        }
        return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::NETWORK_ERROR, UserNetworkInfo());
    }   

    PrinterNetworkResult<UserNetworkInfo> rtcTokenResult = network->getRtcToken();
    if (!rtcTokenResult.isSuccess()) {
        // Pass request context for validation
        checkUserAuthStatus(requestUserInfo, rtcTokenResult.code);
    }
    return rtcTokenResult;
}

PrinterNetworkResult<PrinterNetworkInfo> UserNetworkManager::bindWANPrinter(const PrinterNetworkInfo& printerNetworkInfo)
{
    CHECK_INITIALIZED(PrinterNetworkResult<PrinterNetworkInfo>(PrinterNetworkErrorCode::NOT_INITIALIZED, PrinterNetworkInfo()));
    std::shared_ptr<IUserNetwork> network = getNetwork();
    UserNetworkInfo requestUserInfo = network->getUserNetworkInfo();
    if (!network) {
        if(requestUserInfo.userId.empty()) {
            return PrinterNetworkResult<PrinterNetworkInfo>(PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD, PrinterNetworkInfo());
        }
        return PrinterNetworkResult<PrinterNetworkInfo>(PrinterNetworkErrorCode::NETWORK_ERROR, PrinterNetworkInfo());
    }
    // Record request context before sending request

    PrinterNetworkResult<PrinterNetworkInfo> result = network->bindWANPrinter(printerNetworkInfo);
    if (!result.isSuccess()) {
        // Pass request context for validation
        checkUserAuthStatus(requestUserInfo, result.code);
    }
    return result;
    
}
PrinterNetworkResult<bool> UserNetworkManager::unbindWANPrinter(const std::string& serialNumber)
{
    CHECK_INITIALIZED(PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NOT_INITIALIZED, false));
    std::shared_ptr<IUserNetwork> network = getNetwork();
    UserNetworkInfo requestUserInfo = getUserInfo();
    if (!network) {
        if(requestUserInfo.userId.empty()) {
            return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD, false);
        }
        return PrinterNetworkResult<bool>(PrinterNetworkErrorCode::NETWORK_ERROR, false);
    }
    PrinterNetworkResult<bool> result = network->unbindWANPrinter(serialNumber);
    if (!result.isSuccess()) {
        // Pass request context for validation
        checkUserAuthStatus(requestUserInfo, result.code);
    }
    return result;
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> UserNetworkManager::getUserBoundPrinters()
{
    CHECK_INITIALIZED(PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::NOT_INITIALIZED, std::vector<PrinterNetworkInfo>()));

    std::unique_lock<std::timed_mutex> lock(mMonitorMutex, std::defer_lock);
    if (!lock.try_lock_for(std::chrono::seconds(3))) {
        wxLogMessage("getUserBoundPrinters: Failed to acquire lock after 3 seconds, user network busy");
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::USER_NETWORK_BUSY, std::vector<PrinterNetworkInfo>());
    }

    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        UserNetworkInfo userInfo = getUserInfo();
        if(userInfo.userId.empty()) {
            return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD, std::vector<PrinterNetworkInfo>());
        }
        return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::NETWORK_ERROR, std::vector<PrinterNetworkInfo>());
    }
    
    // Record request context before sending request
    UserNetworkInfo requestUserInfo = network->getUserNetworkInfo();
    
    PrinterNetworkResult<std::vector<PrinterNetworkInfo>> printersResult = network->getUserBoundPrinters();
    if (!printersResult.isSuccess()) {
        // Pass request context for validation
        checkUserAuthStatus(requestUserInfo, printersResult.code);
    }
    return printersResult;
}

void UserNetworkManager::checkUserAuthStatus(const UserNetworkInfo& requestUserInfo, const PrinterNetworkErrorCode& errorCode)
{
    UserNetworkInfo currentUserInfo = getUserInfo();
    
    // 1. Check if user changed
    if (currentUserInfo.userId != requestUserInfo.userId) {
        wxLogMessage("User id is different, skip check network error, request userId: %s, current userId: %s", 
                     requestUserInfo.userId.c_str(), currentUserInfo.userId.c_str());
        return;
    }
    
    // 2. Check if token was changed (more precise than time check)
    if (!requestUserInfo.token.empty() && !currentUserInfo.token.empty() && requestUserInfo.token != currentUserInfo.token) {
        wxLogMessage("Token was changed after this request, ignore stale error (request token: %s..., current token: %s...)",
                     requestUserInfo.token.substr(0, 10).c_str(), currentUserInfo.token.substr(0, 10).c_str());
        return;
    }
    
    // 3. Check if token was refreshed after this request was sent (lastTokenRefreshTime is different)
    if (currentUserInfo.lastTokenRefreshTime != requestUserInfo.lastTokenRefreshTime) {
        wxLogMessage("Token was refreshed after this request (request time: %llu, current refresh time: %llu), "
                     "ignore stale error", 
                     requestUserInfo.lastTokenRefreshTime, currentUserInfo.lastTokenRefreshTime);
        return;
    }
     
    // 4. Parse and update login status
    LoginStatus loginStatus = parseLoginStatusByErrorCode(errorCode);
    if (loginStatus != currentUserInfo.loginStatus) {
        if (loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED || 
            loginStatus == LOGIN_STATUS_OFFLINE ||
            loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN) {
            wxLogMessage("Update user login status from %d to %d due to network error code %d", 
                         currentUserInfo.loginStatus, loginStatus, static_cast<int>(errorCode));
            updateUserInfoLoginStatus(requestUserInfo, loginStatus);
        }
    }
}
UserNetworkInfo UserNetworkManager::getUserInfo() const
{   
    CHECK_INITIALIZED(UserNetworkInfo());

    std::lock_guard<std::mutex> userLock(mUserMutex);
    return mUserInfo;
}

void UserNetworkManager::logout()
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    mUserInfo = UserNetworkInfo();
    saveUserInfo(mUserInfo);
    notifyUserInfoUpdated();
    if (mUserNetwork) {
        mUserNetwork->logout();
        mUserNetwork = nullptr;
    }
}
void UserNetworkManager::login(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    mUserInfo = userInfo;
    mUserInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;
    saveUserInfo(mUserInfo);
    notifyUserInfoUpdated();
    mUserNetwork = nullptr; 
}

std::shared_ptr<IUserNetwork> UserNetworkManager::getNetwork() const
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    return mUserNetwork;
}
void UserNetworkManager::setNetwork(std::shared_ptr<IUserNetwork> network)
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    mUserNetwork = network;
}
bool UserNetworkManager::updateUserInfo(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    // if the user id is the same, update the user info
    if (mUserInfo.userId == userInfo.userId) {
        bool needNotify = false;
        if (mUserInfo.loginStatus != userInfo.loginStatus) {
            needNotify = true;
            wxLogMessage("User login status updated, user id: %s, login status: %d",
                         userInfo.userId.c_str(), userInfo.loginStatus);
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
    wxLogMessage("User id is different, skip update user info, user id: %s, new user id: %s",
                 mUserInfo.userId.c_str(), userInfo.userId.c_str());
    return false;
}

bool UserNetworkManager::updateUserInfoLoginStatus(const UserNetworkInfo& userInfo, const LoginStatus& loginStatus)
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    if(mUserInfo.userId != userInfo.userId) {
        wxLogMessage("User id is different, skip update login status, user id: %s, new user id: %s, login status: %d",
                     mUserInfo.userId.c_str(), userInfo.userId.c_str(), loginStatus);
        return false;
    }

    if(mUserInfo.token != userInfo.token) {
        wxLogMessage("Token is different, skip update login status, user id: %s, new token: %s, login status: %d",
                     mUserInfo.userId.c_str(), userInfo.token.c_str(), loginStatus);
        return false;
    }

    if(mUserInfo.lastTokenRefreshTime != userInfo.lastTokenRefreshTime) {
        wxLogMessage("Last token refresh time is different, skip update login status, user id: %s, new last token refresh time: %llu, login status: %d",
                     mUserInfo.userId.c_str(), userInfo.lastTokenRefreshTime, loginStatus);
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
        userInfo.region.empty() ||
        userInfo.loginStatus == LOGIN_STATUS_NO_USER ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN ||
        userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER;
}
bool UserNetworkManager::checkTokenTimeInvalid(const UserNetworkInfo& userInfo)
{
    uint64_t nowTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    // refresh token expired
    if (userInfo.refreshTokenExpireTime <= nowTime) {
        wxLogMessage("UserNetworkManager::checkTokenTimeInvalid refresh token is expired, token time is invalid, user id: %s, user login "
                     "status: %d, user nickname: %s, refresh token expire time: %llu",
                     userInfo.userId.c_str(), userInfo.loginStatus, userInfo.nickname.c_str(),
                     static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));
        return true;
    }

    // last token refresh time is greater than access token expire time, time is abnormal
    if (userInfo.lastTokenRefreshTime >= userInfo.accessTokenExpireTime) {
        wxLogMessage("UserNetworkManager::checkTokenTimeInvalid last token refresh time is greater than access token expire time, time is "
                     "abnormal, user id: %s, user login status: %d, user nickname: %s, last token refresh "
                     "time: %llu, access token expire time: %llu, refresh token expire time: %llu",
                     userInfo.userId.c_str(), userInfo.loginStatus, userInfo.nickname.c_str(),
                     static_cast<unsigned long long>(userInfo.lastTokenRefreshTime),
                     static_cast<unsigned long long>(userInfo.accessTokenExpireTime),
                     static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));
        return true;
    }
    // nowTime is less than last token refresh time, time is abnormal
    if (nowTime < userInfo.lastTokenRefreshTime) {
        wxLogMessage("UserNetworkManager::checkTokenTimeInvalid nowTime is less than last token refresh time, time is abnormal, need to "
                     "refresh token, user id: %s, user login status: %d, user nickname: %s, last token refresh time: %llu, nowTime: %llu, "
                     "access token expire time: %llu, refresh token expire time: %llu",
                     userInfo.userId.c_str(), userInfo.loginStatus, userInfo.nickname.c_str(),
                     static_cast<unsigned long long>(userInfo.lastTokenRefreshTime), static_cast<unsigned long long>(nowTime),
                     static_cast<unsigned long long>(userInfo.accessTokenExpireTime),
                     static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));
        return true;
    }
    return false;
}
bool UserNetworkManager::checkNeedRefreshToken(const UserNetworkInfo& userInfo)
{
    // if user login status is invalid token or invalid user, don't need to refresh token
    if (userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN || userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER) {
        wxLogMessage("UserNetworkManager::checkNeedRefreshToken user login status is invalid token or invalid user, don't need to refresh "
                     "token, user id: %s, user nickname: %s, user login status: %d",
                     userInfo.userId.c_str(), userInfo.nickname.c_str(), userInfo.loginStatus);
        return false;
    }

    if (checkTokenTimeInvalid(userInfo)) {
        return false;
    }

    // login status is token expired, need to refresh token
    if (userInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED) {
        wxLogMessage("UserNetworkManager::checkNeedRefreshToken user login status is token expired, need to refresh token, user id: %s, "
                     "user nickname: %s, user login status: %d, access token expire time: %llu, refresh token expire time: %llu",
                     userInfo.userId.c_str(), userInfo.nickname.c_str(), userInfo.loginStatus,
                     static_cast<unsigned long long>(userInfo.accessTokenExpireTime),
                     static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));
        return true;
    }

    // use UTC milliseconds to align with backend timestamps
    uint64_t nowTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    // token expired
    if (userInfo.accessTokenExpireTime <= nowTime) {
        wxLogMessage("UserNetworkManager::checkNeedRefreshToken token expired, user id: %s, user login status: %d, user nickname: %s, "
                     "access token expire time: %llu, refresh token expire time: %llu",
                     userInfo.userId.c_str(), userInfo.loginStatus, userInfo.nickname.c_str(),
                     static_cast<unsigned long long>(userInfo.accessTokenExpireTime),
                     static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));
        return true;
    }

    // Half of the token's validity period has passed since the last refresh
    uint64_t tokenValidDiffTime = userInfo.accessTokenExpireTime - userInfo.lastTokenRefreshTime;
    uint64_t elapsedTokenTime   = (nowTime > userInfo.lastTokenRefreshTime) ? (nowTime - userInfo.lastTokenRefreshTime) : 0ULL;
    if (elapsedTokenTime > tokenValidDiffTime / 2) {
        wxLogMessage("UserNetworkManager::checkNeedRefreshToken token valid time is not enough, user id: %s, user login status: %d, user nickname: %s, access token expire time: %llu, refresh token expire time: %llu",
                     userInfo.userId.c_str(), userInfo.loginStatus, userInfo.nickname.c_str(),
                     static_cast<unsigned long long>(userInfo.accessTokenExpireTime),
                     static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));
        return true;
    }
    return false;
}

UserNetworkInfo UserNetworkManager::refreshToken(const UserNetworkInfo& userInfo)
{
    CHECK_INITIALIZED(UserNetworkInfo());

    std::lock_guard<std::timed_mutex> monitorLock(mMonitorMutex);

    UserNetworkInfo currentUserInfo = getUserInfo();
    std::shared_ptr<IUserNetwork> network = getNetwork();
    
    // redact sensitive tokens in logs; print timestamp fields with correct specifiers
    wxLogMessage("UserNetworkManager::refreshToken start refresh token, user id: %s, login refresh time: %llu, access token expire time: %llu, refresh token expire time: %llu",
                 userInfo.userId.c_str(),
                 static_cast<unsigned long long>(userInfo.lastTokenRefreshTime),
                 static_cast<unsigned long long>(userInfo.accessTokenExpireTime),
                 static_cast<unsigned long long>(userInfo.refreshTokenExpireTime));            

    // if current user id is empty, skip refresh token
    if(currentUserInfo.userId.empty()) {
        wxLogMessage("UserNetworkManager::refreshToken current user id is empty, skip refresh token, current user id: %s", currentUserInfo.userId.c_str());
        return UserNetworkInfo();
    }

    // if user login status is invalid token or invalid user, skip refresh token
    if(currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN || 
        currentUserInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER) {
        wxLogMessage("UserNetworkManager::refreshToken user login status is invalid token or invalid user, skip refresh token, current user id: %s", currentUserInfo.userId.c_str());
        return UserNetworkInfo();
    }


    // if user id changed and user already logged in, return current user info
    if(userInfo.userId != currentUserInfo.userId && currentUserInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS) {
        wxLogMessage("UserNetworkManager::refreshToken user id changed, current user id is not empty and user already logged in, return current user info, user id: %s", userInfo.userId.c_str());
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
        return currentUserInfo;
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
    if (userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN || userInfo.loginStatus == LOGIN_STATUS_OFFLINE_INVALID_USER) {
        return false;
    }

    if (!network) {
        network = NetworkFactory::createUserNetwork(userInfo);
        if (!network) {
            userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
            wxLogMessage("User token refresh failed, network is null, user id: %s", userInfo.userId.c_str());
            return false;
        }
        std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHost::get_print_host_type(userInfo.hostType));
        if(networkHelper) {        
            network->setRegion(userInfo.region, networkHelper->getIotUrl());
        } else {
            userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
            wxLogMessage("User token refresh failed, network helper is null, user id: %s, host type: %s", userInfo.userId.c_str(), userInfo.hostType.c_str());
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
        // set login status to login success
        userInfo.loginStatus            = LOGIN_STATUS_LOGIN_SUCCESS;
        // set last token refresh time to current time milliseconds (UTC) to match backend
        userInfo.lastTokenRefreshTime = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        network->updateUserNetworkInfo(userInfo);
        wxLogMessage("User token refreshed successfully, user id: %s", userInfo.userId.c_str());
        return true;
    } 
    if (refreshResult.code != PrinterNetworkErrorCode::NETWORK_ERROR) {
        // if error code is not network error, token is invalid, need to re-login
        userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_TOKEN;
        wxLogMessage("User token refresh failed, need to re-login, user id: %s, error code: %d",
                     userInfo.userId.c_str(), static_cast<int>(refreshResult.code));
        return false;
    } 
    // if error code is network error, don't need to re-login, just set login status to offline
    if(userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED) {
        userInfo.loginStatus = LOGIN_STATUS_OFFLINE;
    }
    wxLogMessage("User token refresh failed, user id: %s, status: %d, error code: %d, error message: %s",
                 userInfo.userId.c_str(), userInfo.loginStatus, static_cast<int>(refreshResult.code), refreshResult.message.c_str());
    return false;
}


void UserNetworkManager::monitorUserNetwork()
{
    mLastLoopTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    while (mRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - mLastLoopTime).count();

        if (elapsed < 10) {
            continue;
        }

        std::lock_guard<std::timed_mutex> lock(mMonitorMutex);
 
        mLastLoopTime = now;
        UserNetworkInfo               userInfo        = getUserInfo();
        std::shared_ptr<IUserNetwork> network         = getNetwork();
        LoginStatus lastLoginStatus = userInfo.loginStatus;

        // Step 1: Check if need re-login
        if (needReLogin(userInfo)) {
            setNetwork(nullptr);
            continue;
        }
     
        if (checkTokenTimeInvalid(userInfo)) {
            updateUserInfoLoginStatus(userInfo, LOGIN_STATUS_OFFLINE_INVALID_TOKEN);
            setNetwork(nullptr);
            continue;
        }

        // Step 2: Check if user changed
        if (network && network->getUserNetworkInfo().userId != userInfo.userId) {
            setNetwork(nullptr);
            continue;
        }

        // Step 3: Check if need refresh token
        if (checkNeedRefreshToken(userInfo)) {
            if (refreshToken(userInfo, network) && updateUserInfo(userInfo)) {
                setNetwork(network);            
            } else {
                setNetwork(nullptr);
                if (lastLoginStatus != userInfo.loginStatus) {
                    updateUserInfoLoginStatus(userInfo, userInfo.loginStatus);
                }
            }         
            continue;
        }

        // Step 4: If already logged in, sync printers and skip
        if (userInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS && network) {
            continue;
        }
    
        // Step 5: Connect to IoT 
        // set last network to nullptr to disconnect from IoT and avoid race condition
        setNetwork(nullptr);   
        std::shared_ptr<IUserNetwork> newNetwork = NetworkFactory::createUserNetwork(userInfo);
        if (!newNetwork) {
            if (lastLoginStatus != LOGIN_STATUS_OFFLINE_INVALID_USER) {
                updateUserInfoLoginStatus(userInfo, LOGIN_STATUS_OFFLINE_INVALID_USER);
            }
            continue;
        }

        // different regions need different service addresses
        std::shared_ptr<INetworkHelper> networkHelper = NetworkFactory::createNetworkHelper(PrintHost::get_print_host_type(userInfo.hostType ));
        if(networkHelper) {
            newNetwork->setRegion(userInfo.region, networkHelper->getIotUrl());
        } else {
            userInfo.loginStatus    = LOGIN_STATUS_OFFLINE_INVALID_USER;
            if(lastLoginStatus != LOGIN_STATUS_OFFLINE_INVALID_USER) {
                updateUserInfoLoginStatus(userInfo, LOGIN_STATUS_OFFLINE_INVALID_USER);
            }
            wxLogMessage("User connect to IoT failed, network helper is null, user id: %s, host type: %s", userInfo.userId.c_str(), userInfo.hostType.c_str());
            continue;
        }

        auto loginResult = newNetwork->connectToIot(userInfo);
        if (loginResult.isSuccess()) {
            if(loginResult.hasData()) {
                UserNetworkInfo loginUserInfo = loginResult.data.value();         
                if (!loginUserInfo.avatar.empty()) userInfo.avatar = loginUserInfo.avatar;
                if (!loginUserInfo.nickname.empty()) userInfo.nickname = loginUserInfo.nickname;
                if (!loginUserInfo.email.empty()) userInfo.email = loginUserInfo.email;         
    
                userInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;    
                if (updateUserInfo(userInfo)) {
                    setNetwork(newNetwork);
                }
            } else {
                if(lastLoginStatus != LOGIN_STATUS_OFFLINE_INVALID_USER) {
                    updateUserInfoLoginStatus(userInfo, LOGIN_STATUS_OFFLINE_INVALID_USER);
                }
            }
        } else {
            LoginStatus status = parseLoginStatusByErrorCode(loginResult.code);              
            if (lastLoginStatus != status) {
                updateUserInfoLoginStatus(userInfo, status);
            }   
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

        boost::nowide::ofstream file(path.string());
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

        boost::nowide::ifstream file(path.string());
        if (!file.is_open()) {
            return;
        }

        nlohmann::json json;
        file >> json;
        file.close();

        UserNetworkInfo userInfo = convertJsonToUserNetworkInfo(json);

        if (!userInfo.userId.empty()) {
            std::lock_guard<std::mutex> lock(mUserMutex);
            if(userInfo.loginStatus != LOGIN_STATUS_OFFLINE_INVALID_TOKEN &&
                userInfo.loginStatus != LOGIN_STATUS_OFFLINE_INVALID_USER &&
                userInfo.loginStatus != LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED) {
                userInfo.loginStatus = LOGIN_STATUS_NOT_LOGIN;
            }
            mUserInfo               = userInfo;
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load user info: %s", e.what());
    }
}

} // namespace Slic3r
