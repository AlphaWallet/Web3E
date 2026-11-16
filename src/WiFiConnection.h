#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <stddef.h>

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
    bool wifiConnect(const char* ssid, const char* password);
    // Walks the list in order. Restarts if none associate.
    // Already-associated calls return immediately, so this is safe from loop().
    void setupWifi(const WiFiCredentials* networks);
};

#endif
