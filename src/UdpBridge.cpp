#include "UdpBridge.h"
#include "Web3.h"
#include "Util.h"
#include "KeyID.h"

//Bridge defaults - override these with setupConnection
static const char *defaultServername = "james.lug.org.cn";
static const uint16_t defaultPort = 5001;
static const uint16_t topPort = 5004;

UdpBridge::UdpBridge()
{
    packetBuffer = new BYTE[256];
    currentReturnBytes = new BYTE[96];
    apiReturn = new APIReturn();
    serverName = std::string(defaultServername);
    memset(sessionBytes, 0, 8);
    port = defaultPort;
    connectionState = handshake;
    Serial.println("Starting UDP Bridge");
    initRandom = false;
    pongCount = 0;
}

void UdpBridge::setKey(KeyID *key, Web3 *w3)
{
    keyID = key;
    web3 = w3;
}

void UdpBridge::setupConnection(std::string& svName, uint16_t p)
{
    serverName = svName;
    port = p;
}

void UdpBridge::startConnection()
{
    //start default port and server
    begin(port);
}

void UdpBridge::checkClientAPI(BridgeCallback callback)
{
    maintainComms();
    parsePacket();
    int len = read(packetBuffer, 256);

    if (len > 0)
    {
        BYTE type = packetBuffer[0];
        int length = (packetBuffer[1] & 0xFF);
        lastComms = millis();
        if (length > 0)
        {
            switch (type)
            {
            case 0:
                if (isNewSession())
                {
                    memcpy(sessionBytes, packetBuffer + 2, 8);
                    if (!keyID->hasRecoveredKey())
                    {
                        initRandomSeed(sessionBytes);
                        keyID->generatePrivateKey(web3);
                    }
                    else if (!initRandom)
                    {
                        initRandomSeed(sessionBytes);
                        Serial.println("Refresh random");
                        initRandom = true;
                    }

                    connectionState = have_token;
                }
                break;
            case 1:
                //we received a response to signature, should be a confirm session key
                if (length == 8 && connectionValidCountdown == 0 && memcmp(sessionBytes, packetBuffer + 2, 8) == 0)
                {
                    connectionState = confirmed;
                    connectionValidCountdown = 60;
                    memcpy(sessionBytes, packetBuffer + 2, 8);
                    memcpy(verifiedSessionBytes, packetBuffer + 2, 8);
                }
                break;
            case 2:
                //API call
                if (packetBuffer[1] == currentQueryId)
                {
                    //return the stored bytes
                    reSendResponse();
                }
                else
                {
                    currentQueryId = packetBuffer[1];
                    int payloadLength = packetBuffer[2];
                    scanAPI(packetBuffer + 3, apiReturn, payloadLength);
                    callback(apiReturn, this, currentQueryId);
                }
                break;

            case 3: //PONG
                lastComms = millis();
                pongCount++;
                break;
            }
        }
    }
}

void UdpBridge::scanAPI(const BYTE *packet, APIReturn *apiReturn, int payloadLength)
{
    apiReturn->clear();

    int index = 0;

    //read length of API description
    apiReturn->apiName = getArg(packet, index, payloadLength);
    Serial.print("API: ");
    Serial.println(apiReturn->apiName.c_str());

    while (index < payloadLength)
    {
        std::string key = getArg(packet, index, payloadLength);
        std::string param = getArg(packet, index, payloadLength);
        apiReturn->params[key] = param;
        Serial.print("PAIR: ");
        Serial.print(key.c_str());
        Serial.print("     ");
        Serial.println(param.c_str());
    }
}

std::string UdpBridge::getArg(const BYTE *packet, int &index, int payloadLength)
{
    int argLen = packet[index++] & 0xFF;
    std::string retVal = "";
    int endIndex = index + argLen;
    if (endIndex > payloadLength)
        endIndex = payloadLength;
    for (; index < endIndex; index++)
    {
        retVal = retVal + (char)packet[index];
    }

    return retVal;
}

void UdpBridge::reSendResponse()
{
    beginPacket(serverName.c_str(), port);
    write(currentReturnBytes, currentReturnBytesLen);
    endPacket();
}

void UdpBridge::sendRefreshRequest()
{
    packetBuffer[0] = 0x00; //fetch me a random
    memcpy(packetBuffer + 1, sessionBytes, 8);
    packetBuffer[9] = 0x01;
    packetBuffer[10] = 0x00;
    int packetLength = 11;

    beginPacket(serverName.c_str(), port);
    write(packetBuffer, packetLength);
    endPacket();
}

void UdpBridge::sendResponse(std::string resp, int methodId)
{
    packetBuffer[0] = 0x02;
    //add session token
    memcpy(packetBuffer + 1, verifiedSessionBytes, 8);
    packetBuffer[9] = resp.length() + 1;
    packetBuffer[10] = (BYTE)methodId;
    memcpy(packetBuffer + 11, resp.c_str(), resp.length());

    int packetLength = 11 + resp.length();

    memcpy(currentReturnBytes, packetBuffer, packetLength);
    currentReturnBytesLen = packetLength;

    beginPacket(serverName.c_str(), port);
    write(packetBuffer, packetLength);
    endPacket();
}

void UdpBridge::maintainComms()
{
    if (millis() > (lastCheck + 2000))
    {
        lastCheck = millis();

        switch (connectionState)
        {
        case handshake:
            connectionValidCountdown = 0;
            sendRefreshRequest();
            pongCount = 0;
            break;
        case have_token:
            sendSignature();
            break;
        case confirmed:
            connectionValidCountdown--;
            if (connectionValidCountdown <= 0)
            {
                connectionState = handshake;
                memset(sessionBytes, 0, 8);
            }
            if ((connectionValidCountdown%2) == 0)
            {
                sendPing(); //send PING every 4 seconds
            }
            break;
        }

        if (connectionState != handshake && millis() > (lastComms + 30*1000))
        {
            connectionState = handshake;
            memset(sessionBytes, 0, 8);
        }

        if (millis() > lastComms + 180*1000)
        {
            lastComms = millis();
            //advance the port
            port++;
            if (port > topPort)
                port = defaultPort;
            begin(port);
        }
    }
}

void UdpBridge::sendSignature()
{
    int packetLength = 0;
    packetBuffer[0] = 0x01;
    //write signature of session token
    memcpy(packetBuffer + 1, sessionBytes, 8);
    packetBuffer[9] = ETHERS_SIGNATURE_LENGTH;
    keyID->getSignature(packetBuffer + 10, sessionBytes);
    //add session token
    packetLength = 10 + ETHERS_SIGNATURE_LENGTH;

    memcpy(currentReturnBytes, packetBuffer, packetLength);
    currentReturnBytesLen = packetLength;

    beginPacket(serverName.c_str(), port);
    write(packetBuffer, packetLength);
    endPacket();
}

void UdpBridge::initRandomSeed(BYTE* randm) 
{
  //generate array
  uint32_t randBuffer[3];
  uint32_t *initKey = (uint32_t *) randm;
  randBuffer[0] = initKey[0];
  randBuffer[1] = initKey[1];
  randBuffer[2] = micros();
  randomInitFromBuffer(randBuffer, 3);
}

boolean UdpBridge::isNewSession()
{
    return (memcmp(sessionBytes, packetBuffer + 2, 8) != 0 && memcmp(sessionBytes, packetBuffer + 10, 8) == 0);
}

void UdpBridge::sendPing()
{
    packetBuffer[0] = 0x03; //Ping
    //write signature of session token
    memcpy(packetBuffer + 1, verifiedSessionBytes, 8);
    beginPacket(serverName.c_str(), port);
    write(packetBuffer, 9);
    endPacket();
}
