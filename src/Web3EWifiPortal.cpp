#include "Web3EWifiPortal.h"
#include "portal/CaptivePortal.h"
#include "Web3Log.h"
#include <Arduino.h>

Web3EWifiPortal::Web3EWifiPortal() : initialized(false), portalActive(false) {}

bool Web3EWifiPortal::begin() {
    if (!portalBoot.begin()) {
        WEB3E_PRINTLN("[Web3EWifiPortal] PortalBoot begin failed");
        return false;
    }
    initialized = true;
    
    portalBoot.checkAndHandlePowerCycle();
    
    return true;
}

bool Web3EWifiPortal::run(uint32_t timeoutMs) {
    if (!initialized) {
        WEB3E_PRINTLN("[Web3EWifiPortal] Not initialized");
        return false;
    }
    
    portalActive = true;
    bool saved = captivePortalRun(&portalBoot, nullptr, timeoutMs);
    portalActive = false;
    
    return saved;
}

bool Web3EWifiPortal::isActive() const {
    return portalActive;
}

bool Web3EWifiPortal::requestPortal(PortalReason reason) {
    if (!initialized) {
        return false;
    }
    
    portalBoot.setPortalReason(reason);
    portalBoot.setStayInPortal(true);
    return true;
}

PortalReason Web3EWifiPortal::getPortalReason() const {
    if (!initialized) {
        return PORTAL_REASON_NONE;
    }
    return portalBoot.getPortalReason();
}

bool Web3EWifiPortal::shouldOpenPortal() const {
    if (!initialized) {
        return false;
    }
    return portalBoot.shouldStayInPortal();
}
