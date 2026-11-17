#ifndef WIFI_DIAGNOSTICS_H
#define WIFI_DIAGNOSTICS_H

#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>

// WiFi failure stages - stored as uint8_t
enum WiFiFailureStage : uint8_t
{
    STAGE_NONE = 0,
    STAGE_ASSOC = 1,      // Association failed
    STAGE_AUTH = 2,       // Authentication failed
    STAGE_DHCP = 3,       // DHCP failed
    STAGE_DNS = 4,        // DNS resolution failed
    STAGE_TLS = 5,        // TLS handshake failed
    STAGE_PROXY_CONNECT = 6 // Proxy connection failed
};

// Diagnostics data structure (stored in NVS)
struct WiFiDiagnosticsData
{
    uint8_t resetReason;           // esp_reset_reason_t value
    uint16_t bootCountSinceLogin;  // Boots since last successful TCP login
    uint16_t wifiAttemptsToConnect; // WiFi connection attempts before success
    uint32_t connectTimeMs;        // Milliseconds to establish WiFi connection
    uint8_t networkIndexUsed;      // Index of the network that succeeded (0-based)
    uint8_t lastFailureStage;      // Last WiFiFailureStage before success
    uint8_t lastDisconnectReason;  // wifi_err_reason_t from last disconnect
    uint8_t reserved;              // Padding for alignment
};

class WiFiDiagnostics
{
public:
    WiFiDiagnostics();
    
    // Initialize and load diagnostics from NVS
    void begin();
    
    // Record a new boot
    void recordBoot();
    
    // Record WiFi connection attempt
    void recordConnectionAttempt(uint8_t networkIndex);
    
    // Record successful WiFi connection
    void recordConnectionSuccess(uint32_t timeMs, uint8_t networkIndex);
    
    // Record failure stage
    void recordFailureStage(WiFiFailureStage stage);
    
    // Record WiFi disconnect reason
    void recordDisconnectReason(uint8_t reason);
    
    // Record successful TCP bridge login and reset counters
    void recordSuccessfulLogin();
    
    // Get current diagnostics data
    const WiFiDiagnosticsData& getData() const { return data; }
    
    // Check if we have diagnostics to report (boot count > 0)
    bool hasDiagnostics() const { return data.bootCountSinceLogin > 0; }
    
private:
    void save();
    void reset();
    
    Preferences prefs;
    WiFiDiagnosticsData data;
    uint32_t connectionStartMs;
    static const char* NAMESPACE;
};

#endif
