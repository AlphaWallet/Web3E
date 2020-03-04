/**
 * Part 2 of TokenScript example
 * 
 * This demo builds on the example 1 adding cryptography and full but inflexible security
 * To get up and running you will need to:
 * 1. Complete Wallet Bridge 1 example
 * 2. Fill in WiFi credentials.
 * 3. Copy wallet address in AlphaWallet (Settings/My Wallet Address) and add into the 'allowedAccounts' below. You may add other accounts here too.
 * 4. Flash the example and run on your ESP32/8266
 * 5. Find the token in AlphaWallet and click through to 'Action'
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
boolean isValidAccount(std::string address);

Ticker blinkTicker(blink, 500, 6); //ticker to blink the LED

//Very simple test system with predefined accounts. 
//This illustrates a legacy system not using Ethereum but confirms the cryptography is functioning correctly.
//Obtain these from AlphaWallet app from Settings/My Wallet Address and copy them here, or obtain them from the log (look for EC-Addr: ...)
const char* allowedAccounts[] = {"0x0000000000000000000000000000000000000000",
                                 "0x0000000000000000000000000000000000000000"};

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
    Serial.println(challenge.c_str());
    udpBridge->sendResponse(challenge, methodId);
    break;
  case api_checkSignature:
  {
    address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &challenge);
    int intervalTime = strtol(apiReturn->params["interval"].c_str(), NULL, 10);
    Serial.print("EC-Addr: ");
    Serial.println(address.c_str()); //The signer's address, ec-recover'd from the signature and challenge 
    if (isValidAccount(address)) //if address ec-recovered from signature matches one of the specified addresses then pass
    {
      blinkLEDOn = false;
      if (intervalTime > 0)
        blinkTicker.interval(intervalTime);
      blinkTicker.start();
      udpBridge->sendResponse("pass", methodId);
    }
    else
    {
      udpBridge->sendResponse("fail : Invalid account", methodId);
    }
    resetChallenge(); //generate a new challenge after each check.
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
  digitalWrite(BLUE_LED, blinkLEDOn);
  blinkLEDOn = !blinkLEDOn;
}

bool iequals(const string &a, const string &b)
{
  unsigned int sz = a.size();
  if (b.size() != sz)
    return false;
  for (unsigned int i = 0; i < sz; ++i)
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  return true;
}

boolean isValidAccount(std::string address)
{
  int accountsSize = (sizeof(allowedAccounts) / sizeof(*allowedAccounts));
  for (int index = 0; index < accountsSize; index++)
  {
    if (iequals(address, allowedAccounts[index]))
      return true;
  }

  return false;
}