/**
 * Part 1 of TokenScript example
 * 
 * This demo shows a very simple connection between wallet and IoT device
 * To get up and running you will need to:
 * 1. Install AlphaWallet on iOS or Android
 * 2. Copy your wallet address from Settings/My Wallet Address
 * 3. Visit the faucet here: https://goerli-faucet.slock.it/ and paste your address in the box and get some Goerli. You may want to repeat to get 0.1 Goerli.
 * 4. Enable Goerli testnet in Settings/Networks and wait until you have Goerli in your wallet. The contract fee will be about 0.005
 * 5. In Dapp Browser ensure you are on Goerli network (top right network select button) type 'NFT' in the top URL bar and click on NFT Token Factory
 * 6. Fill in Contract Name and Token Symbol with the text you want your test token to appear in the wallet and press Deploy. Other fields are optional.
 * 7. When the contract is deployed and the tokens show up in the wallet you will need to find the contract address. 
 *     Go to Transactions and look for the 'Constructor' call. Click this and find the Token address from the 'To:' field. Make a note of this address eg in Telegram/Saved Messages
 *     If you have iOS: find the latest 'Send' transaction with apparantely 0 eth, click for details then click on 'More Details' and get contract address on the webpage
 * 8. Replace the text "<token addr>" in the bundled .xml file with this token address.
 * 9. Setup the WiFi in this program to your wifi credentials.
 * 10. Flash this program to your ESP32
 * 11. Run the program and study the log for the Address. This is now the Device Address for this device.
 * 12. Copy that address and replace "<device address>" in the bundled .xml file with the Device Address you created in step 11 
 * 13. Send the .xml to your mobile phone via Telegram/Saved Messages or email etc. Click on the xml and open with AlphaWallet.
 * 14. Open AlphaWallet and click on your token you created in step 6.
 * 15. Long click on the token picture and select 'Action'
 * 16. Wait to receive the challenge. Once received, sign the challenge, and once complete you should see the LED on the device blink.
 * 
 * All the groundwork is now done to try the second and third examples. These will only require a simple update to get running.
 * 
 **/

#include "esp_wifi.h"
#include <Arduino.h>
#include <Web3.h>
#include <WiFi.h>
#include <functional>
#include <string>
#include <Ticker.h>
#include <TcpBridge.h>

const char *ssid = "<your SSID>";
const char *password = "<your password";

#define BLUE_LED 22 // Little blue LED on the ESP32 TTGO

TcpBridge *tcpConnection;
Web3 *web3;
KeyID *keyID;
std::string challenge;
boolean blinkLEDOn = false;

void resetChallenge();
void blink();
void setupWifi();
std::string handleTCPAPI(APIReturn *apiReturn);

Ticker blinkTicker(blink, 500, 6, MILLIS); //ticker to blink the LED

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
  web3 = new Web3(KOVAN_ID);
  keyID = new KeyID(web3);
  pinMode(BLUE_LED, OUTPUT);
  setupWifi();
  Initialize(); //init after wifi setup to change startup delay

  tcpConnection = new TcpBridge();
  tcpConnection->setKey(keyID, web3);
  tcpConnection->startConnection();
  resetChallenge();
}

void loop() 
{
  setupWifi();
  delay(1);
  tcpConnection->checkClientAPI(&handleTCPAPI);
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
std::string handleTCPAPI(APIReturn *apiReturn)
{
  std::string address;
  //handle the returned API call
  Serial.println(apiReturn->apiName.c_str());

  switch (s_apiRoutes[apiReturn->apiName.c_str()])
  {
  case api_getChallenge:
    resetChallenge(); //generate a new challenge after each check. Advantage is that we've done a full refresh of the mersenne twister
    Serial.println(challenge.c_str());
    return challenge;
  case api_checkSignature:
  {
    address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &challenge);
    int intervalTime = strtol(apiReturn->params["interval"].c_str(), NULL, 10);
    Serial.print("EC-Addr: ");
    Serial.println(address.c_str());
    blinkLEDOn = false;
    if (intervalTime > 0) blinkTicker.interval(intervalTime);
    blinkTicker.start();
    return std::string("pass");
  }
  default:
    Serial.println("Unknown API route: ");
    Serial.println(apiReturn->apiName.c_str());
    return std::string("fail");
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
  
  Serial.print("Blink: ");
  Serial.println(blinkLEDOn);
  blinkLEDOn = !blinkLEDOn;
}