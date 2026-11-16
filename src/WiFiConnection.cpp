#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "WiFiConnection.h"
#include "Web3Log.h"

bool WiFiConnection::wifiConnect(const char* ssid, const char* password)
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    WEB3E_PRINTLN();
    WEB3E_PRINT("Connecting to ");
    WEB3E_PRINTLN(ssid);

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    WiFi.begin(ssid, password);

    int wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 20)
    {
        delay(500);
        WEB3E_PRINT(".");
        wificounter++;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        WEB3E_PRINT("-");
        return false;
    }
    return true;
}

void WiFiConnection::setupWifi(const WiFiCredentials* networks)
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    delay(100);

    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    bool connected = false;
    for (size_t i = 0; networks[i].ssid != nullptr; i++)
    {
        connected = wifiConnect(networks[i].ssid, networks[i].password);
        if (connected)
            break;
    }

    if (!connected)
    {
        WEB3E_PRINTLN("Restarting ...");
        ESP.restart();
    }

    esp_wifi_set_max_tx_power(78);
    delay(10);

    WEB3E_PRINTLN("");
    WEB3E_PRINTLN("WiFi connected.");
    WEB3E_PRINTLN("IP address: ");
    WEB3E_PRINTLN(WiFi.localIP());
}
