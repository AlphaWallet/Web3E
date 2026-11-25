#ifndef WEB3E_WIFI_PORTAL_H
#define WEB3E_WIFI_PORTAL_H

#include <stdint.h>
#include <stdbool.h>
#include <string>
#include "portal/PortalBoot.h"

class Web3EWifiPortal {
public:
    Web3EWifiPortal();
    
    bool begin();
    bool run(uint32_t timeoutMs = 0, const char* deviceAddress = nullptr);
    bool isActive() const;
    bool requestPortal(PortalReason reason);
    
    void setDeviceAddress(const char* address);
    
    PortalReason getPortalReason() const;
    bool shouldOpenPortal() const;
    
private:
    PortalBoot portalBoot;
    bool initialized;
    bool portalActive;
    std::string deviceAddress;
};

#endif
