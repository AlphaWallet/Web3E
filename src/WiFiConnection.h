#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <stddef.h>
#include "WiFiDiagnostics.h"

struct WiFiCredentials
{
    const char* ssid;
    const char* password;
};

class Web3EWifiPortal;

class WiFiConnection
{
public:
    WiFiConnection();
    
    bool wifiConnect(const char* ssid, const char* password, uint8_t networkIndex);
    
    void setupWifi(const WiFiCredentials* networks);
    
    bool maintain();
    
    void markValidated();
    
    void setConnectionTimeout(uint32_t timeoutMs) { connectionTimeoutMs = timeoutMs; }
    
    void setLostWifiTimeout(uint32_t minutes) { lostWifiTimeoutMinutes = minutes; }
    
    void setWifiPortal(Web3EWifiPortal* portal) { wifiPortal = portal; }
    
    WiFiDiagnostics* getDiagnostics() { return &diagnostics; }
    
private:
    WiFiDiagnostics diagnostics;
    uint32_t connectionTimeoutMs;
    uint32_t lostWifiTimeoutMinutes;
    bool wifiValidated;
    unsigned long lastScanMs;
    unsigned long lostWifiStartMs;
    bool lostWifiTimerActive;
    int savedNetworkCount;
    WiFiCredentials savedNetworks[8];
    Web3EWifiPortal* wifiPortal;
    
    bool attemptReconnect();
    void checkLostWifiWatchdog();
    bool scanForSavedNetworks();
    
    static const uint32_t DEFAULT_TIMEOUT_MS = 30000;
    static const uint32_t DEFAULT_LOST_WIFI_TIMEOUT_MINUTES = 0;
    static const uint32_t SCAN_INTERVAL_MS = 45000;
};

#endif
