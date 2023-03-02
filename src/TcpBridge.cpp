#include "TcpBridge.h"
#include "Web3.h"
#include "Util.h"
#include "KeyID.h"

static const uint16_t defaultPort = 8003;
static const char *hostName = "scriptproxy.smarttokenlabs.com";

#define PACKET_BUFFER_SIZE 512

TcpBridge::TcpBridge()
{
    packetBuffer = new BYTE[PACKET_BUFFER_SIZE];
    apiReturn = new APIReturn();
    port = defaultPort;
    connectionState = unconnected;
    Serial.println("Starting TCP Bridge");
}

void TcpBridge::setKey(KeyID *key, Web3 *w3)
{
    keyID = key;
    web3 = w3;
}

void TcpBridge::startConnection()
{
    // start default port and server
    if (!connect(hostName, port))
    {
        Serial.println("connection failed");
    }
    else
    {
        Serial.println("Connected on TCP Bridge");
        connectionState = handshake;
    }

    lastCheck = 0;
}

void TcpBridge::signChallenge(const BYTE *challenge, int length)
{
    keyID->getSignature(packetBuffer + 21, (uint8_t *)challenge, length);       // write sig
    Util::ConvertHexToBytes(packetBuffer + 1, keyID->getAddress().c_str(), 20); // address
    packetBuffer[0] = 0x07;                                                     // indicate using stright sig (ie not signPersonal)
    int packetLength = 1 + ETHERS_ADDRESS_LENGTH + ETHERS_SIGNATURE_LENGTH;
    //Serial.println(Util::ConvertBytesToHex(packetBuffer, packetLength).c_str());
    write(packetBuffer, packetLength);
}

void TcpBridge::checkClientAPI(TcpBridgeCallback callback)
{
    maintainComms(millis());

    if (!available())
        return;

    int len = read(packetBuffer, PACKET_BUFFER_SIZE);

    if (len > 0)
    {
        BYTE type = packetBuffer[0];
        lastComms = millis();
        std::string result;

        Serial.print("RCV: ");
        Serial.println(len);

        switch (type)
        {
        case 0x02:
            signChallenge(&packetBuffer[1], len - 1);
            // challenge, we sign it
            connectionState = confirmed;
            break;

        case 0x04:
            // API call
            scanAPI(packetBuffer + 1, apiReturn, len - 1);
            result = callback(apiReturn);
            Serial.print("Result ");
            Serial.println(result.c_str());
            sendResponse(result);
            break;

        case 0x06:
            SendKeepAlive();
            break;
        }
    }
}

void TcpBridge::sendResponse(std::string resp)
{
    packetBuffer[0] = 0x08;
    memcpy(packetBuffer + 1, resp.c_str(), resp.length());

    int packetLength = 1 + resp.length();
    write(packetBuffer, packetLength);
}

void TcpBridge::sendRefreshRequest()
{
    // Encode the data string into a byte array.
    BYTE tokenVal[33];
    Util::ConvertHexToBytes(&tokenVal[1], keyID->getAddress().c_str(), 20);
    tokenVal[0] = 0x01;
    //Serial.println(keyID->getAddress().c_str());
    write(tokenVal, 21);
}

void TcpBridge::maintainComms(long currentMillis)
{
    if (currentMillis > (lastCheck + 2000))
    {
        lastCheck = currentMillis;

        if (connected() == 0)
            connectionState = unconnected;

        switch (connectionState)
        {
        case unconnected:
            startConnection();
            break;
        case handshake:
            connectionValidCountdown = 0;
            sendRefreshRequest();
            break;
        case confirmed:
        case have_token:
            break;
        }

        if (connectionState > handshake && currentMillis > (lastComms + 60 * 1000))
        {
            connectionState = handshake;
        }
    }
}

void TcpBridge::SendKeepAlive()
{
    packetBuffer[0] = 0x06;
    write(packetBuffer, 1);
}

void TcpBridge::scanAPI(const BYTE *packet, APIReturn *apiReturn, int payloadLength)
{
    apiReturn->clear();

    int index = 0;

    // read length of API description
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

int TcpBridge::getArglen(const BYTE *packet, int &index)
{
    int byteArgLen = packet[index++] & 0xFF;
    int argLen = byteArgLen;

    while ((byteArgLen & 0xFF) == 0xFF)
    {
        byteArgLen = packet[index++] & 0xFF;
        argLen += byteArgLen;
    }

    return argLen;
}

std::string TcpBridge::getArg(const BYTE *packet, int &index, int payloadLength)
{
    int argLen = getArglen(packet, index);
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