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
    return result;
}
PrinterNetworkResult<bool> ElegooUserNetwork::disconnectFromIot()
{
    return ElegooLink::getInstance()->disconnectFromIot();

}
PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::getRtcToken()
{
    return ElegooLink::getInstance()->getRtcToken();
}

PrinterNetworkResult<UserNetworkInfo> ElegooUserNetwork::refreshToken(const UserNetworkInfo& userInfo)
{
    mUserNetworkInfo = userInfo;
    auto result = ElegooLink::getInstance()->refreshToken(userInfo);
    return PrinterNetworkResult<UserNetworkInfo>(result.code, mUserNetworkInfo, result.message);
}

PrinterNetworkResult<std::vector<PrinterNetworkInfo>> ElegooUserNetwork::getUserBoundPrinters()
{
    return ElegooLink::getInstance()->getUserBoundPrinters();
}

} // namespace Slic3r 

