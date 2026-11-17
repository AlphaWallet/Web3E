#include "TcpBridge.h"
#include "Web3.h"
#include "Util.h"
#include "KeyID.h"
#include <esp_arduino_version.h>
#include <esp_timer.h>
#include "Web3Log.h"

static esp_timer_handle_t startupWatchdogTimer = nullptr;

static void onStartupWatchdog(void *)
{
    WEB3E_PRINTLN("Startup watchdog: not confirmed after 30s, restarting");
    ESP.restart();
}

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

void TcpBridge::armStartupWatchdog()
{
    if (startupWatchdogTimer)
        return;

    esp_timer_create_args_t args = {};
    args.callback = &onStartupWatchdog;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "tcp_startup";

    if (esp_timer_create(&args, &startupWatchdogTimer) != ESP_OK)
    {
        WEB3E_PRINTLN("Startup watchdog: timer create failed");
        return;
    }
    if (esp_timer_start_once(startupWatchdogTimer, (uint64_t)STARTUP_WATCHDOG_MS * 1000ULL) != ESP_OK)
    {
        WEB3E_PRINTLN("Startup watchdog: timer start failed");
        return;
    }
    WEB3E_PRINTLN("Startup watchdog armed: 30s");
}

void TcpBridge::disarmStartupWatchdog()
{
    if (!startupWatchdogTimer)
        return;
    esp_timer_stop(startupWatchdogTimer);
}

TcpBridge::TcpBridge()
{
    port = defaultPort;
    strncpy(proxyHost, defaultHost, MAX_PROXY_HOST_LENGTH);
    proxyHost[MAX_PROXY_HOST_LENGTH] = '\0';
    firmwareVersionLen = 0;
    firmwareVersion[0] = '\0';
    wifiDiagnostics = nullptr;
    diagnosticsSent = false;
#ifdef FW_VERSION
    setFirmwareVersion(FW_VERSION);
#endif
    WEB3E_PRINT("Starting TCP Bridge: ");
    WEB3E_PRINT(proxyHost);
    WEB3E_PRINT(":");
    WEB3E_PRINTLN(port);
    connectionValidCountdown = 20;
    resetConnection();
}

void TcpBridge::resetConnection()
{
    connectionState = unconnected;
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

    WEB3E_PRINT("Firmware version set: ");
    WEB3E_PRINTLN(firmwareVersion);
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
    WEB3E_PRINT("TCP Bridge proxy set: ");
    WEB3E_PRINT(proxyHost);
    WEB3E_PRINT(":");
    WEB3E_PRINTLN(port);
}

void TcpBridge::startConnection()
{
    // start default port and server
    if (!connect(proxyHost, port))
    {
        WEB3E_PRINTLN("connection failed");
    }
    else
    {
        WEB3E_PRINTLN("Connected on proxy service");
        connectionState = handshake;
    }

    checkReset();
}

void TcpBridge::checkReset()
{
    if (connectionValidCountdown <= 0)
    {
        ESP.restart();
    }
    connectionValidCountdown--;
}

void TcpBridge::checkConnection()
{
    if (connectionState != unconnected && connected() == 0)
    {
        resetConnection();
    }
}

void TcpBridge::connectionCheck()
{
    switch (connectionState)
    {
    case ConnectionStage::unconnected:
    case ConnectionStage::handshake:
    case ConnectionStage::have_token:
        if (unconnectedTimer == 0)
        {
            unconnectedTimer = millis();
        }
        else
        {
            if (millis() - unconnectedTimer > CONNECTION_TIMEOUT_MS)
            {
                WEB3E_PRINTLN("Unconnected for too long, restarting");
                ESP.restart();
            }
        }
        break;
    case ConnectionStage::confirmed:
        unconnectedTimer = 0;
        break;
    }
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
        WEB3E_PRINTLN(Util::ConvertBytesToHex(packetBuffer, packetLength).c_str());
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
        WEB3E_PRINT("Sending challenge response with firmware version (opcode 0x09): ");
        WEB3E_PRINTLN(firmwareVersion);
        write(packetBuffer, packetLength);
    }
}

void TcpBridge::checkClientAPI(TcpBridgeCallback callback)
{
    maintainComms(millis());
    connectionCheck();

    if (!available())
        return;

    int len = read(packetBuffer, TCP_PACKET_BUFFER_SIZE);

    if (len > 0)
    {
        BYTE type = packetBuffer[0];
        lastComms = millis();
        std::string result;

        switch (type)
        {
        case 0x02:
            signChallenge(&packetBuffer[1], len - 1);
            // challenge, we sign it
            connectionState = confirmed;
            sessionStart = millis();
            connectionValidCountdown = 20;
            disarmStartupWatchdog();
            WEB3E_PRINTLN("Connected to Script service");
            
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
            scanAPI(packetBuffer + 1, &apiReturn, len - 1);
            result = callback(&apiReturn);
            WEB3E_PRINT("Result ");
            WEB3E_PRINTLN(result.c_str());
            sendResponse(result);
            break;

        case 0x06:
            SendKeepAlive();
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
    int retries = 500;
    // Encode the data string into a byte array.
    BYTE tokenVal[33];
    Util::ConvertHexToBytes(&tokenVal[1], keyID->getAddress().c_str(), 20);
    tokenVal[0] = 0x01;
    while (!connected() && (retries-- > 0))
    {
        delay(10);
    }

    if (retries <= 0)
    {
        WEB3E_PRINTLN("Failed to connect to server");
        resetConnection();
        return;
    }

    delay(10);
    write(tokenVal, 21);
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    clear();
#else
    flush();
#endif
}

void TcpBridge::maintainComms(long currentMillis)
{
    if (currentMillis > lastCheck + 2000)
    {
        lastCheck = currentMillis;

        checkConnection();

        switch (connectionState)
        {
        case unconnected:
            startConnection();
            break;
        case handshake:
            sendRefreshRequest();
            break;
        case confirmed:
        case have_token:
            if (currentMillis > (sessionStart + SESSION_TIMEOUT_MS))
            {
                connectionState = handshake;
                sessionStart = 0;
            }
            break;
        }

        if (connectionState > handshake && currentMillis > (lastComms + KEEP_ALIVE_INTERVAL_MS))
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
    
    WEB3E_PRINTLN("Sending WiFi diagnostics (opcode 0x0A):");
    WEB3E_PRINT("  Reset reason: ");
    WEB3E_PRINTLN(data.resetReason);
    WEB3E_PRINT("  Boot count: ");
    WEB3E_PRINTLN(data.bootCountSinceLogin);
    WEB3E_PRINT("  WiFi attempts: ");
    WEB3E_PRINTLN(data.wifiAttemptsToConnect);
    WEB3E_PRINT("  Connect time: ");
    WEB3E_PRINT(data.connectTimeMs);
    WEB3E_PRINTLN("ms");
    WEB3E_PRINT("  Network index: ");
    WEB3E_PRINTLN(data.networkIndexUsed);
    WEB3E_PRINT("  Last failure stage: ");
    WEB3E_PRINTLN(data.lastFailureStage);
    WEB3E_PRINT("  Last disconnect reason: ");
    WEB3E_PRINTLN(data.lastDisconnectReason);
    
    write(packetBuffer, packetLength);
}

void TcpBridge::scanAPI(const BYTE *packet, APIReturn *apiReturn, int payloadLength)
{
    apiReturn->clear();
    int index = 0;

    // read length of API description
    apiReturn->apiName = getArg(packet, index, payloadLength);
    WEB3E_PRINT("API: ");
    WEB3E_PRINTLN(apiReturn->apiName.c_str());

    while (index < payloadLength)
    {
        std::string key = getArg(packet, index, payloadLength);
        std::string param = getArg(packet, index, payloadLength);
        apiReturn->params[key] = param;
        WEB3E_PRINT("PAIR: ");
        WEB3E_PRINT(key.c_str());
        WEB3E_PRINT("     ");
        WEB3E_PRINTLN(param.c_str());
    }
}

std::string TcpBridge::getArg(const BYTE *packet, int &index, int payloadLength)
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
