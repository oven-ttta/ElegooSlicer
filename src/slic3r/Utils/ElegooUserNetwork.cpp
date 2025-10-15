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


PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::loginWAN(const UserNetworkInfo& userInfo)
{
    mUserNetworkInfo = userInfo;
    auto result = ElegooLink::getInstance()->loginWAN(userInfo);
    if(result.isSuccess() && result.hasData()) {
        mUserNetworkInfo = result.data.value();
        mUserNetworkInfo.loginStatus = LOGIN_STATUS_LOGIN_SUCCESS;
    } else {
        mUserNetworkInfo.loginStatus = LOGIN_STATUS_LOGIN_FAILED;
    }
    return result;
}

PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::getRtcToken()
{
    return ElegooLink::getInstance()->getRtcToken();
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooUserNetwork::getPrinters()
{
    return ElegooLink::getInstance()->getPrinters();
}

} // namespace Slic3r 

