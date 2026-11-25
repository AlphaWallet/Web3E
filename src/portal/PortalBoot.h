#ifndef PORTAL_BOOT_H
#define PORTAL_BOOT_H

#include <stdint.h>
#include <stdbool.h>

#ifndef NATIVE_TEST
#include "esp_system.h"
#endif

#define PORTAL_BOOT_NVS_NAMESPACE   "web3e_boot"
#define PORTAL_BOOT_KEY_STAY_PORTAL "stay_portal"
#define PORTAL_BOOT_KEY_BOOT_COUNT  "boot_count"

#define PORTAL_BOOT_TRIPLE_THRESHOLD 3
#define PORTAL_BOOT_STABLE_MS        10000UL

enum PortalReason {
    PORTAL_REASON_NONE = 0,
    PORTAL_REASON_REVOKE = 1,
    PORTAL_REASON_POWER_CYCLE = 2,
    PORTAL_REASON_LOST_WIFI = 3
};

class PortalBoot {
public:
    bool begin();
    bool checkAndHandlePowerCycle();
    bool shouldStayInPortal() const;
    bool setStayInPortal(bool stay);
    bool clearStayInPortal();
    void clearBootCountIfStable(unsigned long uptimeMs);
    void clearBootCountNow();
    
    bool isBootCountCleared() const { return bootCountCleared; }
    uint8_t getBootCount() const;
    bool isInitialized() const { return initialized; }
    
    PortalReason getPortalReason() const;
    bool setPortalReason(PortalReason reason);

private:
    bool initialized = false;
    bool bootCountCleared = false;
    uint8_t bootCount = 0;
    bool stayPortal = false;

    bool readState();
    bool writeStayPortal(uint8_t value);
    bool writeBootCount(uint8_t value);
};

#endif
