#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <stddef.h>
#include "WiFiDiagnostics.h"

// Optional. A sketch that brings its own WiFi never references this file,
// so the linker leaves it out. Call setupWifi before opening TcpBridge.
struct WiFiCredentials
{
    const char* ssid;
    const char* password;
};

class WiFiConnection
{
public:
    WiFiConnection();
    
    bool wifiConnect(const char* ssid, const char* password, uint8_t networkIndex);
    
    // Walks the list in order. Restarts if none associate.
    // Already-associated calls return immediately, so this is safe from loop().
    void setupWifi(const WiFiCredentials* networks);
    
    // Set connection timeout in milliseconds (default 30000 = 30 seconds)
    void setConnectionTimeout(uint32_t timeoutMs) { connectionTimeoutMs = timeoutMs; }
    
    // Get diagnostics tracker
    WiFiDiagnostics* getDiagnostics() { return &diagnostics; }
    
private:
    WiFiDiagnostics diagnostics;
    uint32_t connectionTimeoutMs;
    static const uint32_t DEFAULT_TIMEOUT_MS = 30000; // 30 seconds default
};

#endif
