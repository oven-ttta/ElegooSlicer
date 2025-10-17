#include "ElegooUserNetwork.hpp"
#include "ElegooLink.hpp"
#include "libslic3r/PrinterNetworkResult.hpp"

namespace Slic3r {

ElegooUserNetwork::ElegooUserNetwork(const UserNetworkInfo& userNetworkInfo) : IUserNetwork(userNetworkInfo) {}

ElegooUserNetwork::~ElegooUserNetwork(){


}

void ElegooUserNetwork::uninit()
{
    ElegooLink::getInstance()->uninit();
}

void ElegooUserNetwork::init()
{
    ElegooLink::getInstance()->init();
}


PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::connectToIot(const UserNetworkInfo& userInfo)
{
    UserNetworkInfo userNetworkInfo = userInfo;
    auto result = ElegooLink::getInstance()->connectToIot(userInfo);
    if(result.isSuccess() && result.hasData()) {
        mUserNetworkInfo = result.data.value();
    } else {
        mUserNetworkInfo.loginStatus = LOGIN_STATUS_OFFLINE_INVALID_USER;
        mUserNetworkInfo.connectedToIot = false;
    }
    return result;
}

PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::getRtcToken()
{
    return ElegooLink::getInstance()->getRtcToken();
}

PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::refreshToken(const UserNetworkInfo& userInfo)
{
    auto result = ElegooLink::getInstance()->refreshToken(userInfo);
    if(result.isSuccess() && result.hasData()) {
        UserNetworkInfo userNetworkInfo = result.data.value();
        mUserNetworkInfo.token = userNetworkInfo.token;
        mUserNetworkInfo.refreshToken = userNetworkInfo.refreshToken;
        mUserNetworkInfo.accessTokenExpireTime = userNetworkInfo.accessTokenExpireTime;
        mUserNetworkInfo.refreshTokenExpireTime = userNetworkInfo.refreshTokenExpireTime;
    }
    return result;
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooUserNetwork::getUserBoundPrinters()
{
    return ElegooLink::getInstance()->getUserBoundPrinters();
}

} // namespace Slic3r 

