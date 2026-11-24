#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "WiFiConnection.h"
#include "Web3Log.h"
#include "Web3EWifiPortal.h"

WiFiConnection::WiFiConnection()
{
    connectionTimeoutMs = DEFAULT_TIMEOUT_MS;
    lostWifiTimeoutMinutes = DEFAULT_LOST_WIFI_TIMEOUT_MINUTES;
    wifiValidated = false;
    lastScanMs = 0;
    lostWifiStartMs = 0;
    lostWifiTimerActive = false;
    savedNetworkCount = 0;
    wifiPortal = nullptr;
    
    for (int i = 0; i < 8; i++) {
        savedNetworks[i].ssid = nullptr;
        savedNetworks[i].password = nullptr;
    }
}

bool WiFiConnection::wifiConnect(const char* ssid, const char* password, uint8_t networkIndex)
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    WEB3E_PRINTLN();
    WEB3E_PRINT("Connecting to ");
    WEB3E_PRINTLN(ssid);
    
    diagnostics.recordConnectionAttempt(networkIndex);

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    WiFi.begin(ssid, password);

    uint32_t startMs = millis();
    uint32_t elapsed = 0;
    
    while (WiFi.status() != WL_CONNECTED && elapsed < connectionTimeoutMs)
    {
        delay(500);
        WEB3E_PRINT(".");
        elapsed = millis() - startMs;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        WEB3E_PRINT("-");
        
        // Record failure stage based on WiFi status
        switch (WiFi.status())
        {
            case WL_NO_SSID_AVAIL:
                diagnostics.recordFailureStage(STAGE_ASSOC);
                break;
            case WL_CONNECT_FAILED:
                diagnostics.recordFailureStage(STAGE_AUTH);
                break;
            default:
                diagnostics.recordFailureStage(STAGE_ASSOC);
                break;
        }
        
        return false;
    }
    
    diagnostics.recordConnectionSuccess(elapsed, networkIndex);
    return true;
}

void WiFiConnection::setupWifi(const WiFiCredentials* networks)
{
    if (networks == nullptr) {
        WEB3E_PRINTLN("ERROR: WiFiConnection::setupWifi called with nullptr");
        return;
    }
    
    if (WiFi.status() == WL_CONNECTED)
        return;

    static bool diagnosticsInitialized = false;
    if (!diagnosticsInitialized)
    {
        diagnostics.begin();
        diagnosticsInitialized = true;
    }
    
    savedNetworkCount = 0;
    for (int i = 0; i < 8 && networks[i].ssid != nullptr; i++) {
        savedNetworks[i] = networks[i];
        savedNetworkCount++;
    }

    delay(100);

    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    const int MAX_CYCLES = 3;
    bool connected = false;
    
    for (int cycle = 0; cycle < MAX_CYCLES && !connected; cycle++)
    {
        WEB3E_PRINT("WiFi connection cycle ");
        WEB3E_PRINT(cycle + 1);
        WEB3E_PRINT(" of ");
        WEB3E_PRINTLN(MAX_CYCLES);
        
        for (size_t i = 0; i < savedNetworkCount && !connected; i++)
        {
            connected = wifiConnect(savedNetworks[i].ssid, savedNetworks[i].password, i);
            if (!connected)
            {
                delay(1000);
            }
        }
        
        if (!connected && cycle < MAX_CYCLES - 1)
        {
            delay(2000);
        }
    }

    if (!connected)
    {
        WEB3E_PRINTLN("WiFi watchdog timeout - no networks available after all attempts");
        WEB3E_PRINTLN("Rebooting...");
        delay(1000);
        ESP.restart();
    }

    esp_wifi_set_max_tx_power(78);
    delay(10);

    WEB3E_PRINTLN("");
    WEB3E_PRINTLN("WiFi connected.");
    WEB3E_PRINTLN("IP address: ");
    WEB3E_PRINTLN(WiFi.localIP());
}

bool WiFiConnection::maintain()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (wifiValidated)
        {
            lostWifiTimerActive = false;
            lostWifiStartMs = 0;
        }
        return true;
    }
    
    if (!wifiValidated)
    {
        return attemptReconnect();
    }
    
    checkLostWifiWatchdog();
    
    return false;
}

void WiFiConnection::markValidated()
{
    wifiValidated = true;
    WEB3E_PRINTLN("[WiFi] Connection validated - watchdog armed");
}

bool WiFiConnection::attemptReconnect()
{
    for (int i = 0; i < savedNetworkCount; i++)
    {
        if (wifiConnect(savedNetworks[i].ssid, savedNetworks[i].password, i))
        {
            return true;
        }
    }
    return false;
}

void WiFiConnection::checkLostWifiWatchdog()
{
    unsigned long now = millis();
    
    if (now - lastScanMs < SCAN_INTERVAL_MS)
    {
        return;
    }
    
    lastScanMs = now;
    
    bool savedNetworkVisible = scanForSavedNetworks();
    
    if (savedNetworkVisible)
    {
        lostWifiTimerActive = false;
        lostWifiStartMs = 0;
        
        attemptReconnect();
        return;
    }
    
    int networksFound = WiFi.scanComplete();
    if (networksFound == 0)
    {
        return;
    }
    
    if (!lostWifiTimerActive)
    {
        lostWifiTimerActive = true;
        lostWifiStartMs = now;
        WEB3E_PRINTLN("[WiFi] Lost WiFi watchdog started");
        return;
    }
    
    unsigned long elapsedMinutes = (now - lostWifiStartMs) / 60000UL;
    if (elapsedMinutes >= lostWifiTimeoutMinutes)
    {
        WEB3E_PRINT("[WiFi] No saved network visible for ");
        WEB3E_PRINT(elapsedMinutes);
        WEB3E_PRINTLN(" minutes - opening portal");
        
        if (wifiPortal != nullptr)
        {
            wifiPortal->requestPortal(PORTAL_REASON_LOST_WIFI);
            delay(1000);
            ESP.restart();
        }
    }
}

bool WiFiConnection::scanForSavedNetworks()
{
    WiFi.scanNetworks(false);
    int n = WiFi.scanComplete();
    
    if (n < 0)
    {
        return false;
    }
    
    for (int i = 0; i < n; i++)
    {
        String scannedSSID = WiFi.SSID(i);
        for (int j = 0; j < savedNetworkCount; j++)
        {
            if (scannedSSID == savedNetworks[j].ssid)
            {
                WiFi.scanDelete();
                return true;
            }
        }
    }
    
    WiFi.scanDelete();
    return false;
}
