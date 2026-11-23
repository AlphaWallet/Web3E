/**
 * Send ETH on Sepolia using a cached public RPC pair from universal-node.
 *
 * Web3E no longer ships hardcoded RPC tables or Infura keys. Obtain endpoints
 * with useCachedRpcPair(uninode). Never commit a real PRIVATE_KEY.
 *
 * Setup: Wi-Fi, Sepolia ETH, whitelist device address on uninode, huge_app.
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

const char* ssid = "<Your SSID>";
const char* password = "<Your WiFi password>";
#define MY_ADDRESS "0xDA358D1547238335Cc666E318c511Fed455Ed77C"
#define TARGETADDRESS "0x007bEe82BDd9e866b2bd114780a47f2261C684E3"
#define ETHERSCAN_TX "https://sepolia.etherscan.io/tx/"

const char* PRIVATE_KEY = "DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00";

WiFiConnection wifiConnection;
Web3* web3;
KeyID* keyId;
UniversalNode* uninode;

void sendEthToAddress(double eth, const char* destination);
double queryAccountBalance(const char* address);

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

    double balance = queryAccountBalance(MY_ADDRESS);
    Serial.print("My Address: ");
    Serial.println(MY_ADDRESS);
    Serial.print("Balance of Address: ");
    Serial.println(balance);

    Serial.println("Try to send 0.1 ETH");
    Serial.print("To Address: ");
    Serial.println(TARGETADDRESS);

    if (balance >= 0.11) {
        sendEthToAddress(0.1, TARGETADDRESS);
    } else {
        Serial.println("Balance is insufficient, cannot send ... Go to Eth faucet.");
    }
}

double queryAccountBalance(const char* address) {
    string addressStr = address;
    uint256_t balance = web3->EthGetBalance(&addressStr);
    string balanceStr = Util::ConvertWeiToEthString(&balance, 18);
    Serial.print("Balance: ");
    Serial.println(balanceStr.c_str());
    return atof(balanceStr.c_str());
}

void sendEthToAddress(double eth, const char* destination) {
    Contract contract(web3, "");
    contract.SetPrivateKey(PRIVATE_KEY);
    unsigned long long gasPriceVal = 22000000000ULL;
    uint32_t gasLimitVal = 90000;

    uint256_t weiValue = Util::ConvertToWei(eth, 18);
    string emptyString = "";
    string destinationAddress = destination;
    string fromAddress = MY_ADDRESS;

    uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&fromAddress);
    string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal,
                                             &destinationAddress, &weiValue, &emptyString);
    string transactionHash = web3->getString(&result);
    Serial.print(ETHERSCAN_TX);
    Serial.println(transactionHash.c_str());
}

void loop() {
    web3->maintainCachedRpcPair();
    delay(10000);
}
