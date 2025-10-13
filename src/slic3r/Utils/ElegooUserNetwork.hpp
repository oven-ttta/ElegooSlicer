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

    virtual PrinterNetworkResult<UserNetworkInfo> loginWAN(const UserNetworkInfo& userInfo) override;
    virtual PrinterNetworkResult<UserNetworkInfo> getRtcToken() override;
    virtual PrinterNetworkResult<std::vector<PrinterNetworkInfo>> getPrinters() override;

    virtual void init() override;
    virtual void uninit() override;
};

} // namespace Slic3r

#endif
