/**
 * Push ETH / ERC20 transactions and query an ERC721 balance.
 *
 * RPC endpoints come from universal-node (useCachedRpcPair) — Web3E no longer
 * ships hardcoded per-chain tables or Infura keys. Never commit a real PRIVATE_KEY.
 */

#include <Arduino.h>
#include <WiFiConnection.h>
#include <Web3.h>
#include <KeyID.h>
#include <UniversalNode.h>
#include <Util.h>
#include <Contract.h>
#include <string>

using namespace std;

const char* ssid = "< your WiFi SSID >";
const char* password = "< your WiFi password >";
#define MY_ADDRESS "0x5F89E7FBa60ff6a98fB002c1854E8f29C8b446Cf"
#define ERC20CONTRACT "0xd46c787493418a6a55bd2e01f2a3d598c1a0da08"
#define TARGETADDRESS "0xA20efc4B9537d27acfD052003e311f762620642D"
#define VITALIKADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"
#define CHILLIFROGS "0xa3b7cee4e082183e69a03fc03476f28b12c545a7"
#define ETHERSCAN_TX "https://sepolia.etherscan.io/tx/"

const char* PRIVATE_KEY = "DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00";

WiFiConnection wifiConnection;
Web3* web3;
KeyID* keyId;
UniversalNode* uninode;

void PushERC20Transaction(double tokens, const char* toAddr, const char* contractAddr);
void queryERC721Balance(const char* userAddress);
void sendEthToAddress(double eth, const char* destination);

static bool initCachedPair(Web3* w3, UniversalNode* node) {
    if (!node->syncTime()) {
        Serial.println("WARNING: NTP sync failed — will use EEPROM cache if present.");
        Serial.println(node->lastError());
    }
    w3->useCachedRpcPair(node);
    if (!w3->maintainCachedRpcPair()) {
        Serial.println("WARNING: no usable cached RPC pair yet.");
        Serial.println(node->lastError());
        return false;
    }
    return true;
}

void setup() {
    Serial.begin(115200);

    WiFiCredentials networks[] = {
        {ssid, password},
        {nullptr, nullptr}
    };
    wifiConnection.setupWifi(networks);

    web3 = new Web3(SEPOLIA_ID);
    keyId = new KeyID(web3);
    uninode = new UniversalNode(web3, keyId);

    Serial.print("Device address (whitelist on universal-node): ");
    Serial.println(keyId->getAddress().c_str());

    initCachedPair(web3, uninode);

    sendEthToAddress(0.01, TARGETADDRESS);
    PushERC20Transaction(10, TARGETADDRESS, ERC20CONTRACT);
    queryERC721Balance(MY_ADDRESS);
}

void loop() {
    web3->maintainCachedRpcPair();
    delay(10000);
}

void sendEthToAddress(double eth, const char* destination) {
    Contract contract(web3, "");
    contract.SetPrivateKey(PRIVATE_KEY);
    unsigned long long gasPriceVal = 2200000000LL;
    uint32_t gasLimitVal = 23000;
    string address = MY_ADDRESS;
    uint256_t weiValue = Util::ConvertToWei(eth, 18);
    string emptyString = "";
    string destinationAddress = destination;

    uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&address);
    string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal,
                                             &destinationAddress, &weiValue, &emptyString);
    string transactionHash = web3->getResult(&result);
    Serial.print(ETHERSCAN_TX);
    Serial.println(transactionHash.c_str());
}

void PushERC20Transaction(double tokens, const char* toAddr, const char* contractAddr) {
    string contractAddrStr = contractAddr;
    Contract contract(web3, contractAddr);
    contract.SetPrivateKey(PRIVATE_KEY);
    string address = MY_ADDRESS;

    string param = contract.SetupContractData("name()", &toAddr);
    string result = contract.ViewCall(&param);
    Serial.println(Util::InterpretStringResult(web3->getString(&result).c_str()).c_str());

    param = contract.SetupContractData("decimals()", &toAddr);
    result = contract.ViewCall(&param);
    int decimals = web3->getInt(&result);

    unsigned long long gasPriceVal = 2200000000LL;
    uint32_t gasLimitVal = 4300000;
    uint256_t weiValue = Util::ConvertToWei(tokens, decimals);
    string toAddress = toAddr;
    uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&address);
    uint256_t callValue = 0;
    string p = contract.SetupContractData("transfer(address,uint256)", &toAddress, &weiValue);
    result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal,
                                      &contractAddrStr, &callValue, &p);
    string transactionHash = web3->getResult(&result);
    Serial.print(ETHERSCAN_TX);
    Serial.println(transactionHash.c_str());
}

void queryERC721Balance(const char* /*userAddress*/) {
    // Separate Web3 for mainnet; reuse the same device KeyID.
    Web3* mainWeb3 = new Web3(MAINNET_ID);
    UniversalNode* mainUninode = new UniversalNode(mainWeb3, keyId);
    initCachedPair(mainWeb3, mainUninode);

    Contract contract(mainWeb3, CHILLIFROGS);
    string myAddr = VITALIKADDRESS;
    string param = contract.SetupContractData("balanceOf(address)", &myAddr);
    string result = contract.ViewCall(&param);
    Serial.print("ERC721 balance: ");
    Serial.println(mainWeb3->getInt(&result));

    param = contract.SetupContractData("name()");
    result = contract.ViewCall(&param);
    Serial.print("NAME: ");
    Serial.println(mainWeb3->getString(&result).c_str());
}
