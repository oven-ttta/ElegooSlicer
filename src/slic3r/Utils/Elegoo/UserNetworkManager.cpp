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
    IUserNetwork::init();
    loadUserInfo();

    mRunning.store(true);
    mMonitorThread = std::thread([this]() { monitorLoop(); });
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
        clearBoundPrinters();
        IUserNetwork::uninit();
        mIsInitialized.store(false);
    } 
}

PrinterNetworkResult<UserNetworkInfo> UserNetworkManager::getRtcToken()
{
    CHECK_INITIALIZED(PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::NOT_INITIALIZED, UserNetworkInfo()));
    
    std::shared_ptr<IUserNetwork> network = getNetwork();
    if (!network) {
        UserNetworkInfo userInfo = getUserInfo();
        if(userInfo.userId.empty()) {
            return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::INVALID_USERNAME_OR_PASSWORD, UserNetworkInfo());
        }
        return PrinterNetworkResult<UserNetworkInfo>(PrinterNetworkErrorCode::NETWORK_ERROR, UserNetworkInfo());
    }
    return network->getRtcToken();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> UserNetworkManager::getUserBoundPrinters()
{
    CHECK_INITIALIZED(PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::NOT_INITIALIZED, std::vector<PrinterNetworkInfo>()));
    
    auto printers = getBoundPrinters();
    return PrinterNetworkResult<std::vector<PrinterNetworkInfo>>(PrinterNetworkErrorCode::SUCCESS, printers);
}

UserNetworkInfo UserNetworkManager::getUserInfo() const
{   
    CHECK_INITIALIZED(UserNetworkInfo());

    std::lock_guard<std::mutex> userLock(mUserMutex);
    return mUserInfo;
}

void UserNetworkManager::setUserInfo(const UserNetworkInfo& userInfo)
{
    std::lock_guard<std::mutex> lock(mUserMutex);

    mUserInfo = userInfo; 

    if (mUserNetwork) {
        mUserNetwork->disconnectFromIot();
        mUserNetwork = nullptr;
    }
    
    clearBoundPrinters();
    
    notifyUserInfoUpdated();
    saveUserInfo(userInfo);
}

std::shared_ptr<IUserNetwork> UserNetworkManager::getNetwork() const
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    return mUserNetwork;
}

void UserNetworkManager::setNetwork(std::shared_ptr<IUserNetwork> network)
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    if (!network && mUserNetwork) {
        mUserNetwork->disconnectFromIot();    
        clearBoundPrinters();
    }
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

bool UserNetworkManager::updateUserInfoLoginStatus(const LoginStatus& loginStatus, const std::string& userId)
{
    std::lock_guard<std::mutex> lock(mUserMutex);
    if(mUserInfo.userId != userId) {
        wxLogMessage("User id is different, skip update login status, user id: %s, login status: %d",
                     userId.c_str(), loginStatus);
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

std::vector<PrinterNetworkInfo> UserNetworkManager::getBoundPrinters() const
{
    std::lock_guard<std::mutex> lock(mBoundPrintersMutex);
    return mBoundPrintersList;
}

void UserNetworkManager::setBoundPrinters(const std::vector<PrinterNetworkInfo>& printers)
{
    std::lock_guard<std::mutex> lock(mBoundPrintersMutex);
    mBoundPrintersList = printers;
}

void UserNetworkManager::clearBoundPrinters()
{
    std::lock_guard<std::mutex> lock(mBoundPrintersMutex);
    mBoundPrintersList.clear();
}

void UserNetworkManager::syncBoundPrinters(std::shared_ptr<IUserNetwork> network)
{
    if (!network) {
        clearBoundPrinters();
        return;
    }   
    auto printersResult = network->getUserBoundPrinters();
    if (printersResult.isSuccess() && printersResult.hasData()) {
        setBoundPrinters(printersResult.data.value());
    } else if (!printersResult.isSuccess()) {
        clearBoundPrinters();
        LoginStatus loginStatus = parseLoginStatusByErrorCode(printersResult.code);
        if (loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED || 
            loginStatus == LOGIN_STATUS_OFFLINE ||
            loginStatus == LOGIN_STATUS_OFFLINE_INVALID_TOKEN) {
            updateUserInfoLoginStatus(loginStatus, network->getUserNetworkInfo().userId);
        }
    }
}
void UserNetworkManager::monitorLoop()
{
    mLastLoopTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    while (mRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - mLastLoopTime).count();

        if (elapsed < 10) {
            continue;
        }

        std::lock_guard<std::mutex> lock(mMonitorMutex);
 
        mLastLoopTime = now;
        UserNetworkInfo               userInfo        = getUserInfo();
        std::shared_ptr<IUserNetwork> network         = getNetwork();
        LoginStatus lastLoginStatus = userInfo.loginStatus;

        // Step 1: Check if need re-login
        if (needReLogin(userInfo)) {
            setNetwork(nullptr);
            continue;
        }

        // Step 2: Check if user changed
        if (network && network->getUserNetworkInfo().userId != userInfo.userId) {
            setNetwork(nullptr);
            continue;
        }

        // Step 3: If already logged in, sync printers and skip
        if (userInfo.loginStatus == LOGIN_STATUS_LOGIN_SUCCESS && network) {
            syncBoundPrinters(network);
            continue;
        }

        setNetwork(nullptr);   
        std::shared_ptr<IUserNetwork> newNetwork = NetworkFactory::createUserNetwork(userInfo);
        if (!newNetwork) {
            if (lastLoginStatus != LOGIN_STATUS_OFFLINE_INVALID_USER) {
                updateUserInfoLoginStatus(LOGIN_STATUS_OFFLINE_INVALID_USER, userInfo.userId);
            }
            continue;
        }
        // Step 4: set region
        // different regions need different service addresses
        newNetwork->setRegion(userInfo.region);

        // Step 5: Handle token expired - try to refresh
        if (userInfo.loginStatus == LOGIN_STATUS_OFFLINE_TOKEN_EXPIRED) {
            if (refreshToken(userInfo, newNetwork)) {
                userInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;
                if (updateUserInfo(userInfo)) {
                    setNetwork(newNetwork);
                    syncBoundPrinters(newNetwork);
                } else {
                    newNetwork->disconnectFromIot();
                    newNetwork = nullptr;
                }
            } else {
                if (lastLoginStatus != userInfo.loginStatus) {
                    updateUserInfoLoginStatus(userInfo.loginStatus, userInfo.userId);
                }
            }
            continue;
        }

        // Step 6: Try to connect to IoT        
        auto loginResult = newNetwork->connectToIot(userInfo);
        if (loginResult.isSuccess() && loginResult.hasData()) {
            UserNetworkInfo loginUserInfo = loginResult.data.value();
            
            if (!loginUserInfo.avatar.empty()) userInfo.avatar = loginUserInfo.avatar;
            if (!loginUserInfo.nickname.empty()) userInfo.nickname = loginUserInfo.nickname;
            if (!loginUserInfo.email.empty()) userInfo.email = loginUserInfo.email;
            
            userInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;
            
            if (updateUserInfo(userInfo)) {
                setNetwork(newNetwork);
                syncBoundPrinters(newNetwork);
            } else {
                newNetwork->disconnectFromIot();
                newNetwork = nullptr;
            }
        } else {
            LoginStatus status = parseLoginStatusByErrorCode(loginResult.code);
            if (loginResult.isSuccess()) {
                newNetwork->disconnectFromIot();
                newNetwork = nullptr;
                status = LOGIN_STATUS_OFFLINE_INVALID_USER;
            }                
            if (lastLoginStatus != status) {
                updateUserInfoLoginStatus(status, userInfo.userId);
            }   
        }
    }
}

UserNetworkInfo UserNetworkManager::refreshToken(const UserNetworkInfo& userInfo)
{
    CHECK_INITIALIZED(UserNetworkInfo());

    std::lock_guard<std::mutex> monitorLock(mMonitorMutex);

    UserNetworkInfo currentUserInfo = getUserInfo();
    std::shared_ptr<IUserNetwork> network = getNetwork();
    
    wxLogMessage("UserNetworkManager::refreshToken start refresh token, user id: %s, user token: %s, user refresh token: %s, user login "
                 "refresh time: %d, user access token expire time: %d, user refresh token expire time: %d",
                 userInfo.userId.c_str(), userInfo.token.c_str(), userInfo.refreshToken.c_str(), userInfo.lastTokenRefreshTime,
                 userInfo.accessTokenExpireTime, userInfo.refreshTokenExpireTime);            

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
        // set last token refresh time to current time milliseconds(utc)
        userInfo.lastTokenRefreshTime =  static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
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
