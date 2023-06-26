#ifndef TCP_BRIDGE_H
#define TCP_BRIDGE_H
#include <APIReturn.h>
#include <Web3.h>
#include <WiFi.h>
#include <string>

class TcpBridge;
//Format for API handler is void YourAPIHandler(APIReturn *apiReturn, UdpBridge *client, int methodId) { ... }
typedef std::string (*TcpBridgeCallback)(APIReturn *);

class TcpBridge : public WiFiClient
{
public:
    TcpBridge();
    void startConnection();
    void setKey(KeyID *keyId, Web3 *w3);
    void checkClientAPI(TcpBridgeCallback callback);

    int getConnectionStatus() { return connectionValidCountdown; }
    int getPort() { return port; }

    long getLastCommsTime();

private:
    void scanAPI(APIReturn *apiReturn, int available);
    std::string getArg(int &index);
    std::string bufferToString(const BYTE *packet, int index);
    void closeConnection();
    void SendKeepAlive();
    void maintainComms(long currentMillis);
    void sendRefreshRequest();
    void signChallenge(const BYTE *challenge, int length);
    inline boolean isNewSession();
    void sendPing();
    void sendResponse(std::string resp);
    int  getArgLen(int &index);

    Web3 *web3;
    KeyID *keyID;
    APIReturn *apiReturn;

    BYTE *packetBuffer;

    ConnectionStage connectionState;
    long lastCheck = 0;
    long lastComms = 0;
    int connectionValidCountdown = 0;
    uint16_t port;
};

#endif