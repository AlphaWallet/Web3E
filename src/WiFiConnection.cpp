#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "WiFiConnection.h"
#include "Web3Log.h"

WiFiConnection::WiFiConnection()
{
    connectionTimeoutMs = DEFAULT_TIMEOUT_MS;
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
    if (WiFi.status() == WL_CONNECTED)
        return;

    // Initialize diagnostics on first call
    static bool diagnosticsInitialized = false;
    if (!diagnosticsInitialized)
    {
        diagnostics.begin();
        diagnosticsInitialized = true;
    }

    delay(100);

    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    // Watchdog: Try all networks up to 3 full cycles, then reboot
    const int MAX_CYCLES = 3;
    bool connected = false;
    
    for (int cycle = 0; cycle < MAX_CYCLES && !connected; cycle++)
    {
        WEB3E_PRINT("WiFi connection cycle ");
        WEB3E_PRINT(cycle + 1);
        WEB3E_PRINT(" of ");
        WEB3E_PRINTLN(MAX_CYCLES);
        
        for (size_t i = 0; networks[i].ssid != nullptr && !connected; i++)
        {
            connected = wifiConnect(networks[i].ssid, networks[i].password, i);
            if (!connected)
            {
                delay(1000); // Wait before trying next network
            }
        }
        
        if (!connected && cycle < MAX_CYCLES - 1)
        {
            delay(2000); // Wait before next cycle
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
