#ifndef UDP_BRIDGE_H
#define UDP_BRIDGE_H
#include <APIReturn.h>
#include <Web3.h>
#include <WiFiUdp.h>
#include <string>

enum ConnectionStage
{
    handshake,
    have_token,
    confirmed
};

class UdpBridge;
//Format for API handler is void YourAPIHandler(APIReturn *apiReturn, UdpBridge *client, int methodId) { ... }
typedef void (*BridgeCallback)(APIReturn *, UdpBridge *, int methodId);

class UdpBridge : public WiFiUDP
{
public:
    UdpBridge();
    void startConnection();
    void setKey(KeyID *keyId, Web3 *w3);
    void checkClientAPI(BridgeCallback callback);
    void setupConnection(std::string& serverName, uint16_t port);
    void sendResponse(std::string resp, int methodId);

    int getConnectionStatus() { return connectionValidCountdown; }
    int getConnectionPongs() { return (int) pongCount; }
    int getPort() { return port; }

private:
    void scanAPI(const BYTE *packet, APIReturn *apiReturn, int payloadLength);
    std::string getArg(const BYTE *packet, int &index, int payloadLength);
    void closeConnection();
    void reSendResponse();
    void sendRefreshRequest();
    void maintainComms();
    void sendSignature();
    void initRandomSeed(BYTE* randm); 
    inline boolean isNewSession();
    void sendPing();

    Web3 *web3;
    KeyID *keyID;
    APIReturn *apiReturn;

    BYTE *packetBuffer;
    BYTE *currentReturnBytes;
    int currentReturnBytesLen;

    BYTE sessionBytes[8];
    BYTE verifiedSessionBytes[8];
    BYTE *packetSendBuffer;
    ConnectionStage connectionState;
    long lastCheck = 0;
    long lastComms = 0;
    int connectionValidCountdown = 0;
    BYTE currentQueryId; //ensure UDP packet gets through
    BYTE pongCount = 0;
    boolean initRandom;

    std::string serverName;
    uint16_t port;
};

#endif