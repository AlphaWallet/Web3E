/**
 * Part 3 of TokenScript example
 * 
 * This demo builds on the example 2 adding Token checking.
 * You can now transfer a token to another account and that account will be able to operate the device.
 * To get up and running you will need to:
 * 1. Complete Wallet Bridge 1 example to set up your contract
 * 2. Fill in WiFi credentials.
 * 3. Find the contract address from Example 1, setp 7 and add into the constant TOKEN_CONTRACT below
 * 4. Flash the example and run on your ESP32/8266
 * 5. Find the token in AlphaWallet and click through to 'Action'
 * 6. Create a new account in AlphaWallet.
 * 7. Transfer the token to that account (you will have to copy the destination account to clipboard, then go to the previous account and transfer)
 * 8. Verify the new account with the token can operate the device
 **/

#include "esp_wifi.h"
#include <Arduino.h>
#include <Web3.h>
#include <WiFi.h>
#include <functional>
#include <string>
#include <Ticker.h>

const char *ssid = "<your SSID>";
const char *password = "<your password";

const char *INFURA_HOST = "goerli.infura.io";
const char *INFURA_PATH = "/v3/c7df4c29472d4d54a39f7aa78f146853";

//This is the address of the ERC875 Token Contract you created
#define TOKEN_CONTRACT "0x0000000000000000000000000000000000000000" 

#define BLUE_LED 22 // Little blue LED on the ESP32 TTGO

UdpBridge *udpConnection;
Web3 web3(INFURA_HOST, INFURA_PATH);
KeyID *keyID;
std::string challenge;
boolean blinkLEDOn = false;

void resetChallenge();
void blink();
void setupWifi();
void handleUDPAPI(APIReturn *apiReturn, UdpBridge *udpBridge, int methodId);
bool hasToken(const std::string& userAddress);
bool hasValue(BYTE *value, int length);

Ticker blinkTicker(blink, 500, 6); //ticker to blink the LED

//define API routes
enum APIRoutes
{
  api_unknown,
  api_getChallenge,
  api_checkSignature,
  api_End
};

std::map<std::string, APIRoutes> s_apiRoutes;

//Format for API call from wallet/utilitiy using the bridge is <bridge server address>/<IoT device ethereum address>/<API route>?<arg1>=<your data>&<arg2>=<your data> etc.
//When you call the API this is the extension you use EG www.bridgeserver.com/0x123456789ABCDEF0000/getChallenge
void Initialize()
{
  s_apiRoutes["getChallenge"] = api_getChallenge; 
  s_apiRoutes["checkSignature"] = api_checkSignature;
  s_apiRoutes["end"] = api_End;

  random32v(micros()); //initial init of random seed AFTER wifi connection - since startup after wifi will be a different value of micros each start cycle
}

void setup() 
{
  Serial.begin(115200);
  keyID = new KeyID(&web3);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(BLUE_LED, 0);
  setupWifi();
  Initialize(); //init after wifi setup to change startup delay

  udpConnection = new UdpBridge();
  udpConnection->setKey(keyID, &web3);
  udpConnection->startConnection();
  resetChallenge();
}

void loop() 
{
  setupWifi();
  delay(1);
  udpConnection->checkClientAPI(&handleUDPAPI);
  blinkTicker.update();
}

/* This routine is specifically geared for ESP32 perculiarities */
/* You may need to change the code as required */
/* It should work on 8266 as well */
void setupWifi()
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

  int wificounter = 0;
  while (WiFi.status() != WL_CONNECTED && wificounter < 10)
  {
    delay(500);
    Serial.print(".");
    wificounter++;
  }

  if (wificounter >= 10)
  {
    Serial.println("Restarting ...");
    ESP.restart(); //targetting 8266 & Esp32 - you may need to replace this
  }

  esp_wifi_set_ps(WIFI_PS_NONE);
  delay(10);

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Handle API call from tokenscript
// NB: every path must return a response to the server via udpBridge->sendResponse(<msg>, methodId);
void handleUDPAPI(APIReturn *apiReturn, UdpBridge *udpBridge, int methodId)
{
  std::string address;
  //handle the returned API call
  Serial.println(apiReturn->apiName.c_str());

  switch (s_apiRoutes[apiReturn->apiName.c_str()])
  {
  case api_getChallenge:
    resetChallenge(); //generate a new challenge after each check. Advantage is that we've done a full refresh of the mersenne twister
    Serial.println(challenge.c_str());
    udpBridge->sendResponse(challenge, methodId);
    break;
  case api_checkSignature:
  {
    address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &challenge);
    int intervalTime = strtol(apiReturn->params["interval"].c_str(), NULL, 10);
    Serial.print("EC-Addr: ");
    Serial.println(address.c_str());
    if (hasToken(address)) //Check if address is admin or has token
    {
      blinkLEDOn = false;
      if (intervalTime > 0)
        blinkTicker.interval(intervalTime);
      blinkTicker.start(); //Perform PASS action
      udpBridge->sendResponse("pass", methodId);
    }
    else
    {
      udpBridge->sendResponse("fail : Invalid account", methodId);
    }
  }
  break;
  default:
    Serial.println("Unknown API route: ");
    Serial.println(apiReturn->apiName.c_str());
    udpBridge->sendResponse("fail", methodId);
    break;
  }
}

void resetChallenge()
{
  char buffer[32];
  long challengeVal = random32();
  challenge = "Sign This! ";
  challenge += itoa(challengeVal, buffer, 16);
}

void blink()
{
  blinkLEDOn = !blinkLEDOn;
  digitalWrite(BLUE_LED, blinkLEDOn);
}

bool hasToken(const std::string &userAddress)
{
    boolean hasToken = false;
    const char *contractAddr = TOKEN_CONTRACT;
    Contract contract(&web3, contractAddr);
    string func = "balanceOf(address)";
    string param = contract.SetupContractData(func.c_str(), userAddress);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    BYTE tokenVal[32];

    //break down the result
    vector<string> *vectorResult = Util::InterpretVectorResult(&result);
    for (auto itr = vectorResult->begin(); itr != vectorResult->end(); itr++)
    {
        Util::ConvertHexToBytes(tokenVal, itr->c_str(), 32);
        if (hasValue(tokenVal, 32))
        {
            hasToken = true;
            Serial.println("Has token");
            break;
        }
    }

    delete (vectorResult);
    return hasToken;
}

bool hasValue(BYTE *value, int length)
{
    for (int i = 0; i < length; i++)
    {
        if ((*value++) != 0)
        {
            return true;
        }
    }

    return false;
}