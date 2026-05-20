#include <Arduino.h>
#include <WiFi.h>
#include <Web3.h>
#include <AploContracts.h>
#include <Util.h>

// WiFi credentials - replace with your network details
const char *ssid = "<YOUR_SSID>";
const char *password = "<YOUR_WIFI_PASSWORD>";

// Example AploCoin address - replace with your address
#define APLO_ADDRESS "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb"

Web3 *web3;
int wificounter = 0;

void setup_wifi();
void queryAploBalance();
void queryGaploBalance();

void setup() 
{
    Serial.begin(115200);
    Serial.println("\n\n=== AploEmbed Balance Query Example ===\n");
    
    setup_wifi();
    
    // Initialize Web3 with default AploCoin RPC endpoints
    // Uses pub1.aplocoin.com as primary, pub2.aplocoin.com as fallback
    web3 = new Web3();
    
    // Alternative: specify custom RPC endpoint
    // web3 = new Web3("custom-rpc.aplocoin.com");
    
    // Alternative: specify both primary and fallback
    // web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
    
    Serial.println("Web3 initialized with AploCoin RPC endpoints");
    Serial.println("Primary: pub1.aplocoin.com");
    Serial.println("Fallback: pub2.aplocoin.com\n");
    
    // Query balance in both units
    queryAploBalance();
    queryGaploBalance();
}

void loop() 
{
    // Balance queries are typically done once or on-demand
    // You can add periodic queries here if needed
    delay(10000);
}

/**
 * Query balance in APLO (human-readable format)
 * 1 APLO = 10^18 Gaplo (similar to ETH/wei relationship)
 */
void queryAploBalance()
{
    Serial.println("--- Querying Balance in APLO ---");
    
    string address = APLO_ADDRESS;
    
    // Get balance as human-readable APLO string (18 decimals)
    string balanceAplo = web3->AploGetBalanceInAplo(&address);
    
    Serial.print("Address: ");
    Serial.println(address.c_str());
    Serial.print("Balance: ");
    Serial.print(balanceAplo.c_str());
    Serial.println(" APLO");
    Serial.println();
}

/**
 * Query balance in Gaplo (raw blockchain units / wei equivalent)
 * This is the native unit stored on the blockchain
 */
void queryGaploBalance()
{
    Serial.println("--- Querying Balance in Gaplo (wei) ---");
    
    string address = APLO_ADDRESS;
    
    // Get balance in Gaplo (wei) - raw blockchain units
    uint256_t balanceGaplo = web3->AploGetBalance(&address);
    
    Serial.print("Address: ");
    Serial.println(address.c_str());
    Serial.print("Balance: ");
    Serial.print(balanceGaplo.str().c_str());
    Serial.println(" Gaplo");
    
    // You can also convert manually using Util helper
    string balanceAplo = Util::ConvertWeiToEthString(&balanceGaplo, 18);
    Serial.print("Converted: ");
    Serial.print(balanceAplo.c_str());
    Serial.println(" APLO");
    Serial.println();
}

/**
 * WiFi setup routine for ESP32
 * Adjust as needed for your platform
 */
void setup_wifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
    }

    wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 10)
    {
        for (int i = 0; i < 500; i++)
        {
            delay(1);
        }
        Serial.print(".");
        wificounter++;
    }

    if (wificounter >= 10)
    {
        Serial.println("\nWiFi connection failed. Restarting...");
        ESP.restart();
    }

    delay(10);

    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}
