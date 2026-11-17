#include "WiFiDiagnostics.h"
#include "Web3Log.h"

const char* WiFiDiagnostics::NAMESPACE = "wifidiag";

WiFiDiagnostics::WiFiDiagnostics()
{
    connectionStartMs = 0;
    memset(&data, 0, sizeof(data));
}

void WiFiDiagnostics::begin()
{
    prefs.begin(NAMESPACE, false);
    
    // Load existing diagnostics or initialize
    size_t len = prefs.getBytes("data", &data, sizeof(data));
    if (len != sizeof(data))
    {
        // First boot or corrupted data - initialize
        reset();
        data.resetReason = (uint8_t)esp_reset_reason();
        data.bootCountSinceLogin = 1;
        save();
        WEB3E_PRINTLN("WiFi diagnostics initialized");
    }
    else
    {
        // Existing diagnostics - this is a reboot
        data.bootCountSinceLogin++;
        data.resetReason = (uint8_t)esp_reset_reason();
        save();
        WEB3E_PRINT("WiFi diagnostics loaded, boot count: ");
        WEB3E_PRINTLN(data.bootCountSinceLogin);
    }
}

void WiFiDiagnostics::recordBoot()
{
    data.bootCountSinceLogin++;
    data.resetReason = (uint8_t)esp_reset_reason();
    save();
}

void WiFiDiagnostics::recordConnectionAttempt(uint8_t networkIndex)
{
    if (connectionStartMs == 0)
    {
        connectionStartMs = millis();
        data.wifiAttemptsToConnect = 0;
    }
    data.wifiAttemptsToConnect++;
    data.networkIndexUsed = networkIndex;
}

void WiFiDiagnostics::recordConnectionSuccess(uint32_t timeMs, uint8_t networkIndex)
{
    if (connectionStartMs > 0)
    {
        data.connectTimeMs = millis() - connectionStartMs;
    }
    else
    {
        data.connectTimeMs = timeMs;
    }
    data.networkIndexUsed = networkIndex;
    save();
    
    WEB3E_PRINT("WiFi connected in ");
    WEB3E_PRINT(data.connectTimeMs);
    WEB3E_PRINT("ms on network index ");
    WEB3E_PRINTLN(networkIndex);
}

void WiFiDiagnostics::recordFailureStage(WiFiFailureStage stage)
{
    data.lastFailureStage = (uint8_t)stage;
    save();
}

void WiFiDiagnostics::recordDisconnectReason(uint8_t reason)
{
    data.lastDisconnectReason = reason;
    save();
}

void WiFiDiagnostics::recordSuccessfulLogin()
{
    WEB3E_PRINTLN("TCP bridge login successful - resetting diagnostics counters");
    reset();
    save();
}

void WiFiDiagnostics::save()
{
    prefs.putBytes("data", &data, sizeof(data));
}

void WiFiDiagnostics::reset()
{
    memset(&data, 0, sizeof(data));
    connectionStartMs = 0;
}
