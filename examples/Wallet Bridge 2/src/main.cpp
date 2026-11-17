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
#include <WiFiConnection.h>
#include <functional>
#include <string>
#include <Ticker.h>
#include <TcpBridge.h>

const char *ssid = "<your SSID>";
const char *password = "<your password>";

WiFiConnection wifiConnection;

#define BLUE_LED 22 // Little blue LED on the ESP32 TTGO

TcpBridge *tcpConnection;
Web3 *web3;
KeyID *keyID;
std::string challenge;
boolean blinkLEDOn = false;

void resetChallenge();
void blink();
void setupWifi();
boolean isValidAccount(std::string address);
std::string handleTCPAPI(APIReturn *apiReturn);

Ticker blinkTicker(blink, 500, 6, MILLIS); //ticker to blink the LED

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
  // Arm startup watchdog first (30s timeout until session confirmed)
  TcpBridge::armStartupWatchdog();
  
  Serial.begin(115200);
  
  // Connect to WiFi using WiFiConnection helper
  WiFiCredentials networks[] = {
      {ssid, password},
      {nullptr, nullptr}  // Terminator
  };
  wifiConnection.setupWifi(networks);
  
  web3 = new Web3(SEPOLIA_ID);
  keyID = new KeyID(web3);
  pinMode(BLUE_LED, OUTPUT);
  Initialize(); //init after wifi setup to change startup delay

  tcpConnection = new TcpBridge();
  tcpConnection->setKey(keyID, web3);
  tcpConnection->startConnection();
  resetChallenge();
}

void loop() 
{
  delay(1);
  tcpConnection->checkClientAPI(&handleTCPAPI);
  blinkTicker.update();
}

// Handle API call from tokenscript
// NB: every path must return a response to the server via udpBridge->sendResponse(<msg>, methodId);
std::string handleTCPAPI(APIReturn *apiReturn)
{
  std::string address;
  //handle the returned API call
  Serial.println(apiReturn->apiName.c_str());

  switch (s_apiRoutes[apiReturn->apiName.c_str()])
  {
  case api_getChallenge:
    Serial.println(challenge.c_str());
    return challenge;
  case api_checkSignature:
  {
    address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &challenge);
    int intervalTime = strtol(apiReturn->params["interval"].c_str(), NULL, 10);
    Serial.print("EC-Addr: ");
    Serial.println(address.c_str()); //The signer's address, ec-recover'd from the signature and challenge 
    resetChallenge(); //generate a new challenge after each check.
    if (isValidAccount(address)) //if address ec-recovered from signature matches one of the specified addresses then pass
    {
      blinkLEDOn = false;
      if (intervalTime > 0)
        blinkTicker.interval(intervalTime);
      blinkTicker.start();
      return std::string("pass");
    }
    else
    {
      return std::string("fail : Invalid account");
    }
  }
  break;
  default:
    Serial.println("Unknown API route: ");
    Serial.println(apiReturn->apiName.c_str());
    return std::string("fail");
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