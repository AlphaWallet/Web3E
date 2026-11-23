/**
 * Query ERC20 / ERC721 balances via a cached public RPC pair from universal-node.
 *
 * Web3E no longer ships hardcoded per-chain RPC tables. Obtain endpoints with
 * useCachedRpcPair(uninode) (any chainlist.org chainId uninode serves).
 *
 * Setup: set Wi-Fi credentials, whitelist the printed device address on uninode,
 * flash esp32dev (huge_app), Serial 115200.
 */

#include <Arduino.h>
#include <WiFiConnection.h>
#include <Web3.h>
#include <KeyID.h>
#include <UniversalNode.h>
#include <Contract.h>
#include <Util.h>
#include <string>

using namespace std;

const char* ssid = "<YOUR SSID>";
const char* password = "<YOUR WiFi PASSWORD>";

#define CHILLIFROGS "0xa3b7cee4e082183e69a03fc03476f28b12c545a7"
#define ERC20CONTRACT "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
#define VITALIKADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"

WiFiConnection wifiConnection;
Web3* web3;
KeyID* keyId;
UniversalNode* uninode;

void queryERC721Balance();
void queryERC20Balance();

void setup() {
    Serial.begin(115200);

    WiFiCredentials networks[] = {
        {ssid, password},
        {nullptr, nullptr}
    };
    wifiConnection.setupWifi(networks);

    web3 = new Web3(MAINNET_ID);
    keyId = new KeyID(web3);
    uninode = new UniversalNode(web3, keyId);

    Serial.println("Syncing NTP time (required to fetch signed RPC pair)...");
    if (!uninode->syncTime()) {
        Serial.println("WARNING: NTP sync failed — will use EEPROM cache if present.");
        Serial.println(uninode->lastError());
    }

    Serial.print("Device address (whitelist on universal-node): ");
    Serial.println(keyId->getAddress().c_str());

    web3->useCachedRpcPair(uninode);
    if (!web3->maintainCachedRpcPair()) {
        Serial.println("WARNING: no usable cached RPC pair yet.");
        Serial.println(uninode->lastError());
    }

    queryERC20Balance();
    queryERC721Balance();
}

void loop() {
    WiFiCredentials networks[] = {
        {ssid, password},
        {nullptr, nullptr}
    };
    wifiConnection.setupWifi(networks);
    web3->maintainCachedRpcPair();
    delay(10000);
}

void queryERC721Balance() {
    Contract contract(web3, CHILLIFROGS);
    string myAddr = VITALIKADDRESS;
    string param = contract.SetupContractData("balanceOf(address)", &myAddr);
    string result = contract.ViewCall(&param);

    Serial.print("ERC721 balanceOf: ");
    Serial.println(web3->getInt(&result));

    param = contract.SetupContractData("name()");
    result = contract.ViewCall(&param);
    Serial.print("NAME: ");
    Serial.println(web3->getString(&result).c_str());
}

void queryERC20Balance() {
    string myAddr = VITALIKADDRESS;
    Contract contract(web3, ERC20CONTRACT);

    string param = contract.SetupContractData("balanceOf(address)", &myAddr);
    string result = contract.ViewCall(&param);
    uint256_t baseBalance = web3->getUint256(&result);

    param = contract.SetupContractData("decimals()");
    result = contract.ViewCall(&param);
    int decimals = web3->getInt(&result);

    Serial.print("ERC20 Balance: ");
    Serial.println(Util::ConvertWeiToEthString(&baseBalance, decimals).c_str());

    param = contract.SetupContractData("name()");
    result = contract.ViewCall(&param);
    Serial.print("NAME: ");
    Serial.println(web3->getString(&result).c_str());
}
