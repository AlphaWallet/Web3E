#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WifiServer.h>
#include <Web3.h>
#include <Contract.h>
#include <Crypto.h>
#include <vector>
#include <string>
#include <Util.h>

// This sample should work straight 'out of the box' after you plug in your WiFi credentials

const char *ssid = "<YOUR SSID>";
const char *password = "<YOUR WiFi PASSWORD>";

#define NATIVE_ETH_TOKENS "ETH"                                //if you switch chains you might want to change this
#define CHILLIFROGS "0xa3b7cee4e082183e69a03fc03476f28b12c545a7"
#define ERC20CONTRACT  "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"  //a well known ERC20 token contract on mainnet
#define VITALIKADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"

Web3 *web3;
int wificounter = 0;

void queryERC721Balance();
void queryERC20Balance();
void setup_wifi();
void TestERC20();

void setup() 
{
    Serial.begin(115200); //ensure you set your Arduino IDE port config or platformio.ini with monitor_speed = 115200
    setup_wifi();

    web3 = new Web3(MAINNET_ID);

	queryERC20Balance();
    queryERC721Balance();
}


void loop() 
{
    // put your main code here, to run repeatedly.
}

/* Query balance of Chilli Frogs tokens */
void queryERC721Balance()
{
    web3 = new Web3(MAINNET_ID);
	Contract contract(web3, CHILLIFROGS);

    string myAddr = VITALIKADDRESS;
    string frogs = CHILLIFROGS;

    string func = "balanceOf(address)";
    Serial.println("Start");
    string param = contract.SetupContractData(func.c_str(), &myAddr);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    Serial.print("Balance of Contract ");
    Serial.println(CHILLIFROGS);
    Serial.print("for user: ");
    Serial.print(myAddr.c_str());
    Serial.print(" -> ");

    long bal = web3->getInt(&result);

    Serial.println(bal);

    Serial.println("Fetch Name ... ");

    //Call contract name function
    param = contract.SetupContractData("name()");
    result = contract.ViewCall(&param);
    string contractName = web3->getString(&result);
    Serial.print("NAME: ");
    Serial.println(contractName.c_str());
}

// Query balance of ERC20 token
void queryERC20Balance()
{
    string myAddr = VITALIKADDRESS;
    string usdc = ERC20CONTRACT;

    Contract contract(web3, ERC20CONTRACT);

    string param = contract.SetupContractData("balanceOf(address)", &myAddr);
    string result = contract.ViewCall(&param);

    uint256_t baseBalance = web3->getUint256(&result);

    param = contract.SetupContractData("decimals()");
    result = contract.ViewCall(&param);
    int decimals = web3->getInt(&result);

    string balanceStr = Util::ConvertWeiToEthString(&baseBalance, decimals); //use decimals to calculate value, not all ERC20 use 18 decimals

    Serial.print("ERC20 Balance: ");
    Serial.println(balanceStr.c_str());

    //Call contract name function
    param = contract.SetupContractData("name()");
    result = contract.ViewCall(&param);
    string contractName = web3->getString(&result);
    Serial.print("NAME: ");
    Serial.println(contractName.c_str());
}


/* This routine is specifically geared for ESP32 perculiarities */
/* You may need to change the code as required */
/* It should work on 8266 as well */
void setup_wifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println();
    Serial.print("Connecting to ");
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
        Serial.println("Restarting ...");
        ESP.restart(); //targetting 8266 & Esp32 - you may need to replace this
    }

    delay(10);

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}