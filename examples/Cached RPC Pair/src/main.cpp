/**
 * Cached RPC Pair example
 *
 * Fetches a signed pair of best public RPC URLs for this device's chainId from
 * universal-node (method uninode_getRpcPair), caches them in EEPROM after the
 * WifiStore region, and uses them for normal Web3 RPC with per-request
 * round-robin load-sharing (plus one retry on the other node on transport failure).
 *
 * Unlike setUniversalNode() (which proxies every read through uninode), this
 * mode talks directly to the public nodes. UniversalNode is only used to fetch
 * / refresh the pair (EIP-191 signed request/response, pinned signer).
 *
 * Setup:
 * 1. Set Wi-Fi credentials below (never commit real passwords).
 * 2. Ensure this device address is authorised on universal-node.
 * 3. Flash to esp32dev (huge_app partition). On first boot KeyID generates a
 *    key in EEPROM bytes 0-32; whitelist the printed Device Address.
 * 4. Open Serial at 115200.
 *
 * EEPROM: never touches KeyID (0-32) or WifiStore (33-827). RpcPairStore starts
 * at offset 828. No Wi-Fi passwords or private keys are logged.
 */

#include <Arduino.h>
#include <WiFiConnection.h>
#include <Web3.h>
#include <KeyID.h>
#include <UniversalNode.h>
#include <Util.h>
#include <WiFi.h>
#include <string>

using namespace std;

const char* ssid = "<YOUR SSID>";
const char* password = "<YOUR WiFi PASSWORD>";

#define QUERY_ADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"

WiFiConnection wifiConnection;
Web3* web3;
KeyID* keyId;
UniversalNode* uninode;

static void ensureWifi() {
    WiFiCredentials networks[] = {
        {ssid, password},
        {nullptr, nullptr}
    };
    wifiConnection.setupWifi(networks);
}

static void printCachedRpcs() {
    Serial.print("Cached RPC count: ");
    Serial.println(web3->getCachedRpcCount());
    for (int i = 0; i < web3->getCachedRpcCount(); i++) {
        const char* url = web3->getCachedRpc(i);
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] ");
        // URLs are public endpoints — safe to print. Never print Wi-Fi passwords/keys.
        Serial.println(url ? url : "(null)");
    }
    Serial.print("Pair timestamp: ");
    Serial.println(web3->getCachedRpcTimestamp());
}

void setup() {
    Serial.begin(115200);
    delay(500);

    ensureWifi();

    web3 = new Web3(MAINNET_ID);
    keyId = new KeyID(web3);

    uninode = new UniversalNode(web3, keyId);
    // Default pinned signer: 0xF09b881B849B50D9A3d96b7Aed50f34b5F585A30

    Serial.println("Syncing NTP time (required to fetch signed RPC pair)...");
    if (!uninode->syncTime()) {
        Serial.println("FATAL: NTP sync failed — cannot refresh RPC pair.");
        Serial.println(uninode->lastError());
        // Still try useCachedRpcPair in case EEPROM already has a valid pair
    }

    Serial.print("Device address (whitelist on universal-node): ");
    Serial.println(keyId->getAddress().c_str());
    // Mask: never print KeyID private material (KeyID keeps it internal).

    // Enable cached public-RPC mode. Loads EEPROM cache, refreshes via uninode
    // if empty / wrong chain / older than max age (default 7 days).
    web3->useCachedRpcPair(uninode);

    if (!web3->maintainCachedRpcPair()) {
        Serial.println("WARNING: no usable cached RPC pair yet.");
        Serial.println(uninode->lastError());
    }

    printCachedRpcs();

    string addr = QUERY_ADDRESS;
    Serial.println("\n=== eth_getBalance via cached public RPC ===");
    uint256_t bal = web3->EthGetBalance(&addr);
    Serial.print("Balance wei: ");
    Serial.println(Util::ConvertWeiToEthString(&bal, 18).c_str());

    Serial.println("\nSetup done. loop() maintains WiFi + ~24h pair check.");
}

void loop() {
    // WiFi connect/check loop — setupWifi returns immediately if already associated
    ensureWifi();

    // ~24h throttled refresh check (also runs opportunistically inside Web3::exec)
    web3->maintainCachedRpcPair();

    delay(10000);
}
