#ifndef slic3r_ElegooNetwork_hpp_
#define slic3r_ElegooNetwork_hpp_

#include "PrinterNetwork.hpp"
#include "WSTypes.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

namespace Slic3r {


class ElegooNetwork : public IPrinterNetwork {
public:
    ElegooNetwork(const PrinterInfo& printerInfo);
    virtual ~ElegooNetwork();
    
    virtual bool connect() override;
    virtual void disconnect() override;
    virtual bool isConnected() const override;
    virtual std::vector<PrinterInfo> discoverDevices() override;
    virtual bool sendPrintTask(const PrinterNetworkParams& params) override;
    virtual bool sendPrintFile(const PrinterNetworkParams& params) override;

private:
    PrinterInfo m_printerInfo;
};

} // namespace Slic3r

#endif
