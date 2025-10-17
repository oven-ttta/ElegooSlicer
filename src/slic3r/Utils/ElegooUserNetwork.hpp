#ifndef slic3r_ElegooUserNetwork_hpp_
#define slic3r_ElegooUserNetwork_hpp_

#include "PrinterNetwork.hpp"

namespace Slic3r {
    
class ElegooUserNetwork : public IUserNetwork
{
public:
    ElegooUserNetwork(const UserNetworkInfo& userNetworkInfo);
    ElegooUserNetwork()=delete;
    ElegooUserNetwork(const ElegooUserNetwork&)=delete;
    ElegooUserNetwork& operator=(const ElegooUserNetwork&)=delete;
    virtual ~ElegooUserNetwork();

    virtual PrinterNetworkResult<UserNetworkInfo> connectToIot(const UserNetworkInfo& userInfo) override;
    virtual PrinterNetworkResult<UserNetworkInfo> getRtcToken() override;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> getUserBoundPrinters() override;
    virtual PrinterNetworkResult<UserNetworkInfo> refreshToken(const UserNetworkInfo& userInfo) override;

    static void init();
    static void uninit();
};

} // namespace Slic3r

#endif
