#include "TcpBridge.h"
#include "Web3.h"
#include "Util.h"
#include "KeyID.h"

// Default proxy configuration (overridable via compile-time flags or runtime setter)
// The old smarttokenlabs.com domain no longer resolves - use percolate.one
#if defined(PROXY_HOST)
static const char *defaultHost = PROXY_HOST;
#elif defined(WEB3E_PROXY_HOST)
static const char *defaultHost = WEB3E_PROXY_HOST;
#else
static const char *defaultHost = "scriptproxy.percolate.one";
#endif

#if defined(PROXY_PORT)
static const uint16_t defaultPort = PROXY_PORT;
#elif defined(WEB3E_PROXY_PORT)
static const uint16_t defaultPort = WEB3E_PROXY_PORT;
#else
static const uint16_t defaultPort = 8083;
#endif

TcpBridge::TcpBridge()
{
    port = defaultPort;
    strncpy(proxyHost, defaultHost, MAX_PROXY_HOST_LENGTH);
    proxyHost[MAX_PROXY_HOST_LENGTH] = '\0';
    connectionState = unconnected;
    firmwareVersionLen = 0;
    firmwareVersion[0] = '\0';
    wifiDiagnostics = nullptr;
    diagnosticsSent = false;
#ifdef FW_VERSION
    setFirmwareVersion(FW_VERSION);
#endif
    Serial.print("TCP Bridge: ");
    Serial.print(proxyHost);
    Serial.print(":");
    Serial.println(port);
}

void TcpBridge::setKey(KeyID *key, Web3 *w3)
{
    keyID = key;
    web3 = w3;
}

void TcpBridge::setFirmwareVersion(const char *version)
{
    if (version == nullptr)
    {
        firmwareVersionLen = 0;
        firmwareVersion[0] = '\0';
        return;
    }

    size_t len = strlen(version);
    if (len > MAX_FIRMWARE_VERSION_LENGTH)
    {
        len = MAX_FIRMWARE_VERSION_LENGTH;
    }

    memcpy(firmwareVersion, version, len);
    firmwareVersion[len] = '\0';
    firmwareVersionLen = len;

    Serial.print("Firmware version set: ");
    Serial.println(firmwareVersion);
}

void TcpBridge::setWiFiDiagnostics(WiFiDiagnostics *diag)
{
    wifiDiagnostics = diag;
    diagnosticsSent = false;
}

void TcpBridge::setProxy(const char *host, uint16_t proxyPort)
{
    if (host != nullptr)
    {
        strncpy(proxyHost, host, MAX_PROXY_HOST_LENGTH);
        proxyHost[MAX_PROXY_HOST_LENGTH] = '\0';
    }
    port = proxyPort;
    Serial.print("TCP Bridge proxy set: ");
    Serial.print(proxyHost);
    Serial.print(":");
    Serial.println(port);
}

void TcpBridge::startConnection()
{
    // start default port and server
    if (!connect(proxyHost, port))
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
    if (firmwareVersionLen == 0)
    {
        // Legacy opcode 0x07: no firmware version
        keyID->getSignature(packetBuffer + 21, (uint8_t *)challenge, length);       // write sig
        Util::ConvertHexToBytes(packetBuffer + 1, keyID->getAddress().c_str(), 20); // address
        packetBuffer[0] = 0x07;                                                     // indicate using stright sig (ie not signPersonal)
        int packetLength = 1 + ETHERS_ADDRESS_LENGTH + ETHERS_SIGNATURE_LENGTH;
        //Serial.println(Util::ConvertBytesToHex(packetBuffer, packetLength).c_str());
        write(packetBuffer, packetLength);
    }
    else
    {
        // New opcode 0x09: includes firmware version in signed challenge
        // Build preimage: challenge || versionLen || version
        BYTE preimage[length + 1 + MAX_FIRMWARE_VERSION_LENGTH];
        memcpy(preimage, challenge, length);
        preimage[length] = firmwareVersionLen;
        memcpy(preimage + length + 1, firmwareVersion, firmwareVersionLen);
        int preimageLength = length + 1 + firmwareVersionLen;

        // Sign the preimage
        BYTE signature[ETHERS_SIGNATURE_LENGTH];
        keyID->getSignature(signature, preimage, preimageLength);

        // Build packet: [opcode][address][signature][versionLen][version]
        packetBuffer[0] = 0x09; // new opcode for firmware version
        Util::ConvertHexToBytes(packetBuffer + 1, keyID->getAddress().c_str(), 20); // address
        memcpy(packetBuffer + 21, signature, ETHERS_SIGNATURE_LENGTH);              // signature
        packetBuffer[86] = firmwareVersionLen;                                       // version length
        memcpy(packetBuffer + 87, firmwareVersion, firmwareVersionLen);             // version string

        int packetLength = 1 + ETHERS_ADDRESS_LENGTH + ETHERS_SIGNATURE_LENGTH + 1 + firmwareVersionLen;
        Serial.print("Sending challenge response with firmware version (opcode 0x09): ");
        Serial.println(firmwareVersion);
        write(packetBuffer, packetLength);
    }
}

void TcpBridge::checkClientAPI(TcpBridgeCallback callback)
{
    maintainComms(millis());

    uint16_t rLen = available();

    if (rLen <= 0)
        return;

    Serial.print("Available: ");
    Serial.println(rLen);

    int type = read();

    if (type > 0)
    {
        int len;
        lastComms = millis();
        std::string result;

        switch (type)
        {
        case 0x02:
            Serial.println("Challenge");
            len = read(packetBuffer, PACKET_BUFFER_SIZE);
            signChallenge(packetBuffer, len);
            // challenge, we sign it
            connectionState = confirmed;
            
            // Send diagnostics after successful login (once per boot)
            if (!diagnosticsSent && wifiDiagnostics != nullptr && wifiDiagnostics->hasDiagnostics())
            {
                sendDiagnostics();
                diagnosticsSent = true;
                wifiDiagnostics->recordSuccessfulLogin();
            }
            break;

        case 0x04:
            // API call
            scanAPI(&apiReturn, rLen - 1);
            result = callback(&apiReturn);
            Serial.print("Result ");
            Serial.println(result.c_str());
            sendResponse(result);
            break;

        case 0x06:
            Serial.println("Keep Alive");
            read(packetBuffer, PACKET_BUFFER_SIZE);
            SendKeepAlive();
            break;

        default:
            Serial.print("Unknown: ");
            Serial.println(type);
            break;    
        }
    }
}

// Get Diff from last time we had server comms
long TcpBridge::getLastCommsTime()
{
    return lastComms;
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

void TcpBridge::sendDiagnostics()
{
    if (wifiDiagnostics == nullptr)
        return;
    
    const WiFiDiagnosticsData& data = wifiDiagnostics->getData();
    
    // Build diagnostics packet with opcode 0x0A
    // Wire format: [opcode][resetReason][bootCount(2)][wifiAttempts(2)][connectTimeMs(4)][networkIndex][lastFailureStage][lastDisconnectReason][reserved]
    packetBuffer[0] = 0x0A; // Diagnostics opcode
    
    // Pack the diagnostics data structure
    int offset = 1;
    packetBuffer[offset++] = data.resetReason;
    
    // bootCountSinceLogin (uint16_t, big-endian)
    packetBuffer[offset++] = (data.bootCountSinceLogin >> 8) & 0xFF;
    packetBuffer[offset++] = data.bootCountSinceLogin & 0xFF;
    
    // wifiAttemptsToConnect (uint16_t, big-endian)
    packetBuffer[offset++] = (data.wifiAttemptsToConnect >> 8) & 0xFF;
    packetBuffer[offset++] = data.wifiAttemptsToConnect & 0xFF;
    
    // connectTimeMs (uint32_t, big-endian)
    packetBuffer[offset++] = (data.connectTimeMs >> 24) & 0xFF;
    packetBuffer[offset++] = (data.connectTimeMs >> 16) & 0xFF;
    packetBuffer[offset++] = (data.connectTimeMs >> 8) & 0xFF;
    packetBuffer[offset++] = data.connectTimeMs & 0xFF;
    
    packetBuffer[offset++] = data.networkIndexUsed;
    packetBuffer[offset++] = data.lastFailureStage;
    packetBuffer[offset++] = data.lastDisconnectReason;
    packetBuffer[offset++] = data.reserved;
    
    int packetLength = offset; // Should be 14 bytes total (1 opcode + 13 data bytes)
    
    Serial.println("Sending WiFi diagnostics (opcode 0x0A):");
    Serial.print("  Reset reason: ");
    Serial.println(data.resetReason);
    Serial.print("  Boot count: ");
    Serial.println(data.bootCountSinceLogin);
    Serial.print("  WiFi attempts: ");
    Serial.println(data.wifiAttemptsToConnect);
    Serial.print("  Connect time: ");
    Serial.print(data.connectTimeMs);
    Serial.println("ms");
    Serial.print("  Network index: ");
    Serial.println(data.networkIndexUsed);
    Serial.print("  Last failure stage: ");
    Serial.println(data.lastFailureStage);
    Serial.print("  Last disconnect reason: ");
    Serial.println(data.lastDisconnectReason);
    
    write(packetBuffer, packetLength);
}

void TcpBridge::scanAPI(APIReturn *apiReturn, int available)
{
    int index = 0;
    apiReturn->clear();
    apiReturn->apiName = getArg(index);
    Serial.print("API: ");
    Serial.println(apiReturn->apiName.c_str());

    while (index < available)
    {
        std::string key = getArg(index);
        std::string param = getArg(index);

        apiReturn->params[key] = param;
        /*Serial.print("PAIR: ");
        Serial.print(key.c_str());
        Serial.print("     ");
        Serial.println(param.c_str());
        Serial.print("Index: ");
        Serial.println(index);*/
    }
}

int TcpBridge::getArgLen(int &index)
{
    int byteArgLen = 0xFF;
    int argLen = 0;

    while ((byteArgLen & 0xFF) == 0xFF)
    {
        byteArgLen = read();
        argLen += byteArgLen;
        index++;
    }

    return argLen;
}

std::string TcpBridge::bufferToString(const BYTE *packet, int endIndex)
{
    std::string retVal = "";
    for (int index = 0; index < endIndex; index++)
    {
        retVal = retVal + (char)packet[index];
    }

    return retVal;
}

std::string TcpBridge::getArg(int &index)
{
    int argLen = getArgLen(index);
    std::string retVal = "";

    while (argLen > 0)
    {
        int readAmount = argLen > PACKET_BUFFER_SIZE ? PACKET_BUFFER_SIZE : argLen;
        int len = read(packetBuffer, readAmount);
        index += len;
        argLen -= len;
        retVal += bufferToString(packetBuffer, len);
    }

    return retVal;
}
