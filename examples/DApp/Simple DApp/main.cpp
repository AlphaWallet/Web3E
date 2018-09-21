#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WifiServer.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "SSD1306.h"
#include <Web3.h>
#include <Contract.h>
#include <Crypto.h>
#include <vector>
#include <string>
#include <Util.h>

const char *ssid = "<Your SSID>";
const char *password = "<Your WiFi password>";
const char *INFURA_HOST = "kovan.infura.io";
const char *INFURA_PATH = "/v3/c7df4c29472d4d54a39f7aa78f146853";

#define BLUE_LED 2 //the little blue LED on the ESP8266/ESP32
#define CHALLENGE_SIZE 32

WiFiServer server(80);
int wificounter = 0;

const char * seedWords[] = { "Apples", "Oranges", "Grapes", "Dragon fruit", "Bread fruit", "Pomegranate", "Mangifera indica", "Persea americana", 0 };

String header;
String currentChallenge;

void checkClientHeader(WiFiClient client);
void setup_wifi();
void generateSeed(BYTE *buffer);
void updateChallenge();
void sendPage(WiFiClient client);
bool checkSignature(String signature, String address);
void insertCSS(WiFiClient client);
bool checkReceivedData(String userData);

void setup() 
{
    Serial.begin(115200); //ensure you set your Arduino IDE port config or platformio.ini with monitor_speed = 115200

    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, LOW);

    setup_wifi();
    updateChallenge();
    server.begin();
}

void loop() 
{
    setup_wifi(); //ensure we maintain a connection. This may cause the server to reboot periodically, if it loses connection

    WiFiClient client = server.available(); // Listen for incoming clients

    if (client)
    {
        Serial.println("New Client.");
        checkClientHeader(client);
    }
}

void checkClientHeader(WiFiClient client)
{
    boolean currentLineIsBlank = true;
    String currentLine = ""; // make a String to hold incoming data from the client
    int timeout = 0;

    while (client.connected())
    { // loop while the client's connected
        if (client.available())
        {
            timeout = 0;
            // if there's bytes to read from the client,
            char c = client.read(); // read a byte, then
            Serial.write(c);        // print it out the serial monitor
            header += c;
            if (c == '\n' && currentLineIsBlank)
            {
                // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                // and a content-type so the client knows what's coming, then a blank line:
                sendPage(client);

                // Break out of the while loop
                break;
            }
            if (c == '\n')
            { // if you got a newline, then clear currentLine
                currentLine = "";
                currentLineIsBlank = true;
            }
            else if (c != '\r')
            {
                currentLineIsBlank = false;
                currentLine += c;
            }
        }
        else
        {
            delay(1);
            if (timeout++ > 300)
                break;
            Serial.print(".");
        }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    delay(1);
    client.flush();
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
}

void sendPage(WiFiClient client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    bool passed = false;

    String checkLine = "RCV/";
    String getCheck = "GET /" + checkLine;

    int userDataIndex = header.indexOf(getCheck.c_str());

    // Deal with received user data (we use URL to receive data from the app)
    if (userDataIndex >= 0)
    {
        int sigIndex = userDataIndex + getCheck.length();
        String receivedUserData = header.substring(sigIndex);
        passed = checkReceivedData(receivedUserData);
    }

    // Display the HTML web page
    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    insertCSS(client);
    client.println("</head>");

    // Web Page Heading
    client.println("<body><h1>Web3E Test DApp</h1>");

    if (passed)
    {
        //signature passed!
        client.println("<h2><div id=\"sig\">Signature Passed!</div></h2>");
        //return to home page after 2 seconds
        client.println("<script>function loadDelay2() { setTimeout(function () { window.location.replace('/'); }, 2000);} window.onload = loadDelay2;</script>");
    }
    else
    {

        client.println("<p>Challenge: " + currentChallenge + "</p>");
        client.println("<h4><div id=\"useraddr\">User Account:</div></h4>");
        client.println("<p id=\"activatebutton\" class=\"stormbut2\"></p>");
        client.println("<p id=\"signLayer\"><button onclick=\"signButton()\" id=\"signButton\" class=\"stormbut\">Sign Challenge</button></p>");
        client.println("<div id=\"sig\"></div>");
        //Sign button handling - web3.eth.sign asks AlphaWallet/Metamask to sign the challenge then inserts the received signature and address into URL
        client.println("<script>function signButton() { var accountElement = document.getElementById('useraddr'); var account = web3.eth.coinbase; var container = document.getElementById('message'); var sig = document.getElementById('sig'); var message = '" + currentChallenge + "'; web3.eth.sign(account, message, (err, data) => { sig.innerHTML = data; accountElement.innerHTML = 'Validating Signature and Attestation'; var signButton = document.getElementById(\"signButton\"); signButton.style.visibility = \"hidden\"; var currentLoc = window.location.href; window.location.replace(currentLoc + '" + checkLine + "' + data + '/' + account + '/'); });}</script>");
        client.println("<script>");
        client.println(Web3::getDAppCode());

        //reads the user's ethereum address from web3.eth.coinbase and displays on the page
        client.println("function displayAddress(){ var signButton = document.getElementById('signButton'); var account = web3.eth.coinbase; var accountElement = document.getElementById('useraddr'); accountElement.innerHTML = 'User Account: ' + account; }");
        //half second deley before trying to read coinbase - wait for race condition between starting the page and the injector code populating the address
        client.println("function loadDelay() { setTimeout(function () { displayAddress(); }, 500); } window.onload = loadDelay;");
        client.println("</script>");
    }

    client.println("</body></html>");

    // The HTTP response ends with another blank line
    client.println();
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

void generateSeed(BYTE *buffer)
{
    for (int i = 0; i < CHALLENGE_SIZE; i++)
    {
        buffer[i] = (BYTE)random(0,256);
    }
}

void updateChallenge()
{
    //generate a new challenge
    int size = 0;
    while (seedWords[size] != 0) size++;
    Serial.println(size);
    
    int seedIndex = random(0, size);
    currentChallenge = seedWords[seedIndex];
    currentChallenge += " ";
    currentChallenge += (millis() + random(0,16384)) & 0xFFFF; //NB Do not use this in production systems!
}

bool checkSignature(String signature, String address)
{
    uint8_t challengeHash[ETHERS_KECCAK256_LENGTH];

    //we have to hash the challenge first
    //first convert to a hex string
    string hexVal = Util::ConvertBytesToHex((uint8_t*)currentChallenge.c_str(), currentChallenge.length());
    Serial.println(hexVal.c_str());

    //String signMessage = "\u0019Ethereum Signed Message:\n";
    //signMessage += hexVal.length();
    //signMessage += hexVal.substring(2); // cut off the leading '0x'

    Serial.println("BYTES");
    Serial.println(Util::ConvertBytesToHex((uint8_t*)currentChallenge.c_str(), currentChallenge.length()).c_str());

    //hash the full challenge    
    Crypto::Keccak256((uint8_t*)currentChallenge.c_str(), currentChallenge.length(), challengeHash);

    Serial.println("Challenge : " + currentChallenge);
    Serial.print("Challenge#: ");
    Serial.println(Util::ConvertBytesToHex(challengeHash, ETHERS_KECCAK256_LENGTH).c_str());

    //convert address to BYTE
    BYTE addrBytes[ETHERS_ADDRESS_LENGTH];
    BYTE signatureBytes[ETHERS_SIGNATURE_LENGTH];
    BYTE publicKeyBytes[ETHERS_PUBLICKEY_LENGTH];

    Util::ConvertHexToBytes(addrBytes, address.c_str(), ETHERS_ADDRESS_LENGTH);
    Util::ConvertHexToBytes(signatureBytes, signature.c_str(), ETHERS_SIGNATURE_LENGTH);

    Serial.println("SIG");
    Serial.println(Util::ConvertBytesToHex(signatureBytes, ETHERS_SIGNATURE_LENGTH).c_str());

    Serial.println();

    // Perform ECRecover to get the public key - this extracts the public key of the private key
    // that was used to sign the message - that is, the private key held by the wallet/metamask
    Crypto::ECRecover(signatureBytes, publicKeyBytes, challengeHash); 

    Serial.println("Recover Pub:");
    Serial.println(Util::ConvertBytesToHex(publicKeyBytes, ETHERS_PUBLICKEY_LENGTH).c_str());

    BYTE recoverAddr[ETHERS_ADDRESS_LENGTH];
    // Now reduce the recovered public key to Ethereum address
    Crypto::PublicKeyToAddress(publicKeyBytes, recoverAddr);

    Serial.println("Received Address:");
    Serial.println(address);
    Serial.println("Recovered Address:");
    Serial.println(Util::ConvertBytesToHex(recoverAddr, ETHERS_ADDRESS_LENGTH).c_str());

    if (memcmp(recoverAddr, addrBytes, ETHERS_ADDRESS_LENGTH) == 0) //did the same address sign the message as the user is claiming to own?
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool checkReceivedData(String userData)
{
    int sigEnd = userData.indexOf("/");
    String address = userData.substring(sigEnd + 1);
    int addressEnd = address.indexOf("/");
    String signature = userData.substring(0, sigEnd);
    address = address.substring(0, addressEnd);

    Serial.println("Got Sig: " + signature);
    Serial.println("Got Addr: " + address);

    bool passed = checkSignature(signature, address);
    if (passed)
    {
        Serial.println("Correct Signature obtained!");
        digitalWrite(BLUE_LED, HIGH);
        updateChallenge(); //generate a new challenge
    }
    else
    {
        Serial.println("Signature Fails");
    }

    return passed;
}

void insertCSS(WiFiClient client)
{
    client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    client.println(".stormbut { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
    client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; }");
    client.println(".stormbut2 { background-color: coral; border: none; color: white; padding: 16px 40px;");
    client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; visibility: hidden;}");
    client.println(".button2 {background-color: #77878A;}</style>");
}