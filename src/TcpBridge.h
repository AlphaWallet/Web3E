#ifndef TCP_BRIDGE_H
#define TCP_BRIDGE_H
#include <APIReturn.h>
#include <Web3.h>
#include <WiFi.h>
#include <string>
#include "WiFiDiagnostics.h"

#define CONNECTION_TIMEOUT_MS 60000L
#define SESSION_TIMEOUT_MS (15 * 60 * 1000L)
#define KEEP_ALIVE_INTERVAL_MS (5 * 60 * 1000L)
// From armStartupWatchdog() until the session is confirmed. Fires even if
// setup() is blocked inside WiFi or WiFiClient::connect().
#define STARTUP_WATCHDOG_MS 30000L
#define TCP_PACKET_BUFFER_SIZE 512

class TcpBridge;
//Format for API handler is void YourAPIHandler(APIReturn *apiReturn, UdpBridge *client, int methodId) { ... }
typedef std::string (*TcpBridgeCallback)(APIReturn *);

class TcpBridge : public WiFiClient
{
public:
    TcpBridge();
    /** One-shot. Call at the start of setup(), before WiFi. */
    static void armStartupWatchdog();
    void startConnection();
    void setKey(KeyID *keyId, Web3 *w3);
    void setFirmwareVersion(const char *version);
    void setWiFiDiagnostics(WiFiDiagnostics *diag);
    void setProxy(const char *host, uint16_t port);
    void checkClientAPI(TcpBridgeCallback callback);

    int getConnectionStatus() { return connectionValidCountdown; }
    int getPort() { return port; }
    const char* getHost() { return proxyHost; }

private:
    void scanAPI(const BYTE *packet, APIReturn *apiReturn, int payloadLength);
    std::string getArg(const BYTE *packet, int &index, int payloadLength);
    void SendKeepAlive();
    void maintainComms(long currentMillis);
    void sendRefreshRequest();
    void signChallenge(const BYTE *challenge, int length);
    void sendDiagnostics();
    void sendResponse(std::string resp);
    void checkReset();
    void resetConnection();
    void checkConnection();
    void connectionCheck();
    static void disarmStartupWatchdog();

    Web3 *web3;
    KeyID *keyID;
    WiFiDiagnostics *wifiDiagnostics;
    APIReturn apiReturn;
    static const uint16_t PACKET_BUFFER_SIZE = 512;
    static const uint8_t MAX_FIRMWARE_VERSION_LENGTH = 32;
    static const uint16_t MAX_PROXY_HOST_LENGTH = 128;
    BYTE packetBuffer[PACKET_BUFFER_SIZE];
    char firmwareVersion[MAX_FIRMWARE_VERSION_LENGTH + 1];
    uint8_t firmwareVersionLen;
    char proxyHost[MAX_PROXY_HOST_LENGTH + 1];
    bool diagnosticsSent;

    ConnectionStage connectionState;
    long lastCheck = 0;
    long lastComms = 0;
    int connectionValidCountdown = 0;
    long unconnectedTimer = 0;
    long sessionStart = 0;
    uint16_t port;
};

#endif