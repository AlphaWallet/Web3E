/**
 * WiFi Portal Example
 *
 * Demonstrates the optional Web3E WiFi portal module with:
 * - Triple power-cycle to open portal (offline reset)
 * - Lost-WiFi watchdog that opens portal after 5 minutes
 * - Programmatic revoke API (opens portal on next reboot)
 * - Multi-network support with EEPROM persistence
 *
 * Setup:
 * 1. Flash to ESP32. On first boot it will reboot after failing to find WiFi.
 * 2. Triple power-cycle (power off/on 3 times quickly within 10 seconds).
 * 3. Connect to ESP32-Setup-XXXX AP (open/no password).
 * 4. Browser should auto-open portal at 192.168.4.1, or navigate manually.
 * 5. Add your WiFi network(s) and click Done.
 * 6. Device reboots and connects to saved WiFi.
 *
 * The portal only opens when:
 * - Triple power-cycle detected (stay_portal flag set)
 * - requestPortalOnRevoke() called then device reboots
 * - Lost WiFi watchdog triggers (validated WiFi lost for 5+ minutes)
 *
 * The portal NEVER opens automatically on simple WiFi failure or router outage.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiConnection.h>
#include <WifiStore.h>
#include <KeyID.h>
#include <Web3.h>

#define WEB3E_WIFI_PORTAL
#include <Web3EWifiPortal.h>

WiFiConnection wifiConnection;
WifiStore wifiStore;
Web3EWifiPortal wifiPortal;
KeyID* keyId = nullptr;
Web3* web3 = nullptr;

unsigned long lastMaintainMs = 0;
unsigned long lastStatusPrintMs = 0;

void runPortalAndReboot(const char* deviceAddress)
{
    Serial.println("[Portal] Opening embedded captive portal...");
    Serial.println("[Portal] Connect to ESP32-Setup-XXXX AP (192.168.4.1)");
    
    bool saved = wifiPortal.run(0);
    
    Serial.printf("[Portal] Exiting portal (saved=%d) — rebooting\n", saved ? 1 : 0);
    delay(500);
    ESP.restart();
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Web3E WiFi Portal Example ===");
    Serial.printf("Firmware built: %s %s\n", __DATE__, __TIME__);
    Serial.printf("Reset reason: %d\n", (int)esp_reset_reason());
    Serial.println();
    
    if (!wifiPortal.begin())
    {
        Serial.println("[Portal] WARNING: PortalBoot failed to initialize");
    }
    else
    {
        bool forcePortal = wifiPortal.shouldOpenPortal();
        if (forcePortal)
        {
            PortalReason reason = wifiPortal.getPortalReason();
            const char* reasonStr = "unknown";
            switch (reason)
            {
                case PORTAL_REASON_REVOKE: reasonStr = "revoke"; break;
                case PORTAL_REASON_POWER_CYCLE: reasonStr = "triple power-cycle"; break;
                case PORTAL_REASON_LOST_WIFI: reasonStr = "lost WiFi"; break;
                default: reasonStr = "unspecified"; break;
            }
            Serial.printf("[Portal] stay_portal active (reason: %s)\n", reasonStr);
            
            if (!web3)
                web3 = new Web3(1);
            if (!keyId)
                keyId = new KeyID(web3);
            
            String addr = keyId->getAddress().c_str();
            runPortalAndReboot(addr.c_str());
            return;
        }
    }
    
    Serial.println("Initializing KeyID...");
    if (!web3)
        web3 = new Web3(1);
    if (!keyId)
        keyId = new KeyID(web3);
    
    Serial.print("Device Address: ");
    Serial.println(keyId->getAddress().c_str());
    Serial.println();
    
    Serial.println("Reading WiFi credentials from EEPROM...");
    if (!wifiStore.begin())
    {
        Serial.println("ERROR: WifiStore begin failed");
        delay(3000);
        ESP.restart();
    }
    
    WifiCredential credentials[MAX_WIFI_NETWORKS];
    int networkCount = wifiStore.readNetworks(credentials, MAX_WIFI_NETWORKS);
    
    Serial.printf("Found %d saved network(s)\n", networkCount);
    for (int i = 0; i < networkCount; i++)
    {
        Serial.printf("  %d: %s\n", i + 1, credentials[i].ssid);
    }
    Serial.println();
    
    if (networkCount == 0)
    {
        Serial.println("No WiFi networks saved. Rebooting (triple power-cycle to open portal)...");
        delay(3000);
        ESP.restart();
    }
    
    WiFiCredentials networks[MAX_WIFI_NETWORKS + 1];
    for (int i = 0; i < networkCount; i++)
    {
        networks[i].ssid = credentials[i].ssid;
        networks[i].password = credentials[i].password;
    }
    networks[networkCount].ssid = nullptr;
    networks[networkCount].password = nullptr;
    
    wifiConnection.setWifiPortal(&wifiPortal);
    wifiConnection.setupWifi(networks);
    
    wifiConnection.markValidated();
    
    Serial.println("Setup complete. WiFi validated, watchdog armed.");
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  'r' - Request portal on next reboot (revoke simulation)");
    Serial.println("  's' - Print WiFi status");
    Serial.println();
}

void loop()
{
    unsigned long now = millis();
    
    if (now - lastMaintainMs >= 1000)
    {
        lastMaintainMs = now;
        wifiConnection.maintain();
    }
    
    if (now - lastStatusPrintMs >= 30000)
    {
        lastStatusPrintMs = now;
        Serial.print("WiFi: ");
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("Connected to ");
            Serial.print(WiFi.SSID());
            Serial.print(" (");
            Serial.print(WiFi.localIP());
            Serial.print(") RSSI: ");
            Serial.println(WiFi.RSSI());
        }
        else
        {
            Serial.println("Disconnected (watchdog active)");
        }
    }
    
    if (Serial.available())
    {
        char cmd = Serial.read();
        while (Serial.available()) Serial.read();
        
        switch (cmd)
        {
            case 'r':
            case 'R':
                Serial.println("[Command] Requesting portal on next reboot (revoke simulation)...");
                wifiPortal.requestPortal(PORTAL_REASON_REVOKE);
                Serial.println("[Command] Portal requested. Rebooting in 3 seconds...");
                delay(3000);
                ESP.restart();
                break;
                
            case 's':
            case 'S':
                Serial.println("[Status] WiFi Information:");
                Serial.print("  Status: ");
                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.println("Connected");
                    Serial.print("  SSID: ");
                    Serial.println(WiFi.SSID());
                    Serial.print("  IP: ");
                    Serial.println(WiFi.localIP());
                    Serial.print("  RSSI: ");
                    Serial.println(WiFi.RSSI());
                }
                else
                {
                    Serial.println("Disconnected");
                }
                Serial.print("  Device Address: ");
                Serial.println(keyId->getAddress().c_str());
                Serial.println();
                break;
        }
    }
    
    delay(10);
}
