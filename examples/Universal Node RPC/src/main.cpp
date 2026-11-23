/**
 * Universal Node RPC example
 *
 * Makes read-only JSON-RPC calls through https://uninode.percolate.one
 * (signed proxy). Requests are EIP-191 signed with the device KeyID key;
 * responses are verified against the pinned node signer address so a
 * spoofed/MITM RPC cannot forge an NFT balance. For direct public-RPC reads
 * and writes, prefer examples/Cached RPC Pair (useCachedRpcPair).
 *
 * Setup:
 * 1. Set Wi-Fi credentials below.
 * 2. Ensure this device address is authorised on universal-node
 *    (DEVICE_WHITELIST / config/devices.json).
 * 3. Flash to esp32dev. On first boot KeyID generates a key in EEPROM
 *    (bytes 0-32); copy the printed Device Address into the whitelist.
 * 4. Open Serial at 115200 and watch verified eth_getBalance + ERC-721
 *    balanceOf results.
 *
 * No private keys or API secrets are embedded in this sketch.
 */

#include <Arduino.h>
#include <WiFiConnection.h>
#include <Web3.h>
#include <KeyID.h>
#include <Contract.h>
#include <UniversalNode.h>
#include <Util.h>
#include <string>

using namespace std;

const char* ssid = "<YOUR SSID>";
const char* password = "<YOUR WiFi PASSWORD>";

// Vitalik's address — public query target for eth_getBalance
#define QUERY_ADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"

// Chilli Frogs (ERC-721) on Ethereum mainnet
#define ERC721_CONTRACT "0xa3b7cee4e082183e69a03fc03476f28b12c545a7"

WiFiConnection wifiConnection;
Web3* web3;
KeyID* keyId;
UniversalNode* uninode;

void setup() {
    Serial.begin(115200);
    delay(500);

    WiFiCredentials networks[] = {
        {ssid, password},
        {nullptr, nullptr}
    };
    wifiConnection.setupWifi(networks);

    web3 = new Web3(MAINNET_ID);
    // Recovers or generates device key in EEPROM bytes 0-32 (unchanged layout).
    keyId = new KeyID(web3);

    uninode = new UniversalNode(web3, keyId);
    // Default host: uninode.percolate.one, default signer:
    // 0xF09b881B849B50D9A3d96b7Aed50f34b5F585A30
    // Override if your deployment uses a different node signer:
    // uninode->setNodeSigner("0x...");

    Serial.println("Syncing NTP time (required for signed RPC)...");
    if (!uninode->syncTime()) {
        Serial.println("FATAL: NTP sync failed — cannot use universal-node.");
        Serial.println(uninode->lastError());
        return;
    }

    web3->setUniversalNode(uninode);

    Serial.print("Device address (whitelist this on universal-node): ");
    Serial.println(keyId->getAddress().c_str());
    Serial.print("Pinned node signer: ");
    Serial.println(uninode->nodeSigner());

    // --- eth_getBalance via universal-node ---
    string addr = QUERY_ADDRESS;
    Serial.println("\n=== eth_getBalance (verified) ===");
    uint256_t bal = web3->EthGetBalance(&addr);
    if (strlen(uninode->lastError()) > 0 && bal == uint256_t(0)) {
        Serial.print("FAILED: ");
        Serial.println(uninode->lastError());
    } else {
        Serial.print("Balance wei: ");
        Serial.println(Util::ConvertWeiToEthString(&bal, 18).c_str());
        Serial.print("Pinned block: ");
        Serial.println(uninode->lastBlockNumber().c_str());
    }

    // --- ERC-721 balanceOf via eth_call ---
    Serial.println("\n=== ERC-721 balanceOf (verified) ===");
    Contract contract(web3, ERC721_CONTRACT);
    string owner = QUERY_ADDRESS;
    string param = contract.SetupContractData("balanceOf(address)", &owner);
    string result = contract.ViewCall(&param);
    if (result.empty()) {
        Serial.print("FAILED: ");
        Serial.println(uninode->lastError());
    } else {
        long nftBal = web3->getInt(&result);
        Serial.print("NFT balanceOf(");
        Serial.print(owner.c_str());
        Serial.print("): ");
        Serial.println(nftBal);
        Serial.print("Pinned block: ");
        Serial.println(uninode->lastBlockNumber().c_str());
        Serial.println(result.c_str());
    }

    Serial.println("\nDone.");
}

void loop() {
}
