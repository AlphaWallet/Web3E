#include <Arduino.h>
#include "NimBLEDevice.h"
#include <stdint.h>
#include "mbedtls/aes.h"

static NimBLEUUID readHandshake("bd4ac614-0b45-11e3-8ffd-0800200c9a66");
static NimBLEUUID writeHandshake("bd4ac613-0b45-11e3-8ffd-0800200c9a66");
static NimBLEUUID readSecure("bd4ac612-0b45-11e3-8ffd-0800200c9a66");
static NimBLEUUID writeSecure("bd4ac611-0b45-11e3-8ffd-0800200c9a66");

static NimBLEUUID serviceUUID("0000FE24-0000-1000-8000-00805F9B34FB");
static NimBLEUUID serviceUUID2("fe24"); // sometimes the full code isn't advertised correctly, we also need to try the cut down version

typedef void (*ConnectCallback)(const char *, bool);
typedef void (*DisConnectCallback)();

void scanEndedCB(NimBLEScanResults results);
static DisConnectCallback dsCb = nullptr;
class AdvertisedDeviceCallbacks;

#if 0
#define AUGUST_LOG( format, ... ) printf("August: "#format"\n",##__VA_ARGS__)
#else
#define AUGUST_LOG( format, ... ) (void)format
#endif

typedef enum LockAction
{
    GET_STATUS,
    LOCK,
    UNLOCK,
    TOGGLE_LOCK
} LockAction;

class AugustLock
{
public:
    AugustLock(const char *deviceAddress, const char *handshakeKey, const uint8_t offlineKeyOffset);

    // performs scan, connects to client and does handshake
    void connect(ConnectCallback callback, notify_callback notifyCB, notify_callback secureLockCallback, DisConnectCallback dsCallback);
    void checkStatus();
    void lockAction(LockAction action);
    void init();

    void _notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length);
    void _secureLockCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length);
    
    void setDiscoveredLock(NimBLEAdvertisedDevice *advDev) { advDevice = advDev; doConnect = true; }
    void blankClient() { pClient = nullptr; }

private:
    void resetCrypto();
    void lockCommand(LockAction action);
    void closeConnection();
    bool connectToServer();
    void scanForService();

    uint8_t *BuildCommand(byte opcode);
    uint8_t *lockCmd(uint8_t opcode);
    uint8_t *statusCmd();
    uint8_t getLockCode(LockAction action);
    const char* getLockActionStr(LockAction action);
    void getStatus();

    void encryptMessage(uint8_t *plainText, uint8_t *encryptBuffer);
    void decryptMessage(uint8_t *encryptedMessage, uint8_t *decryptBuffer);
    void cbcEncryptMessage(uint8_t *plainText, uint8_t *encryptBuffer);
    void cbcDecryptMessage(uint8_t *encryptedMessage, uint8_t *decryptBuffer);
    void initSessionKey();
    void getHandshakeBytes(uint8_t *handshakeBytes);
    uint32_t ReadUInt32LE(uint8_t *buffer, int startIndex);
    void WriteUInt32LE(uint8_t *buffer, uint value, int startIndex);
    uint32_t SecSecurityChecksum(uint8_t *buffer);
    void SecWriteChecksum(uint8_t *buffer);
    uint8_t SimpleChecksum(uint8_t *buffer);
    void WriteChecksum(uint8_t *buffer);

    ConnectCallback connectCallback;
    const char *deviceAddress;
    const char *handshakeKey;
    uint8_t *handshakeKeyBytes;
    uint8_t offlineKeyOffset;

    uint8_t handshakeSessionKey[16]; // session key
    uint8_t commandBuffer[18];

    uint8_t iv[16];  // Encrypt IV
    uint8_t iv2[16]; // Decrypt IV
    mbedtls_aes_context lockCypher;

    bool doneConnect = false;
    bool doConnect = false;
    LockAction actionAfterSync = GET_STATUS;

    BLERemoteCharacteristic *pWriteHandshake;
    BLERemoteCharacteristic *pWriteSecure;

    NimBLEClient *pClient = nullptr;
    NimBLEAdvertisedDevice *advDevice = nullptr;
    AdvertisedDeviceCallbacks *deviceCallback;

    notify_callback pNotifyCB;
    notify_callback pSecureLockCallback;
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
private:
    AugustLock * lock;   

public:
    AdvertisedDeviceCallbacks(AugustLock *l) { lock = l; }

    void onResult(NimBLEAdvertisedDevice *advertisedDevice)
    {
        AUGUST_LOG("Advertised Device found: %s", advertisedDevice->toString().c_str());
        if (advertisedDevice->isAdvertisingService(serviceUUID))
        {
            AUGUST_LOG("Found Our Service");
            /** stop scan before connecting */
            NimBLEDevice::getScan()->stop();
            /** Save the device reference in a global for the client to use*/
            lock->setDiscoveredLock(advertisedDevice);
        }
    };
};

class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient)
    {
        AUGUST_LOG("Connected ...");
        pClient->updateConnParams(10, 120, 1, 700); // pClient->updateConnParams(12, 120, 0, 300);
    };

    void onDisconnect(NimBLEClient *pClient)
    {
        AUGUST_LOG("%s: Disconnected", pClient->getPeerAddress().toString().c_str());
        NimBLEDevice::deleteClient(pClient);
        delay(2000);
        if (dsCb != nullptr) dsCb();
    };

    /** Called when the peripheral requests a change to the connection parameters.
     *  Return true to accept and apply them or false to reject and keep
     *  the currently used parameters. Default will return true.
     */
    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        if (params->itvl_min < 24)
        { /** 1.25ms units */
            return false;
        }
        else if (params->itvl_max > 40)
        { /** 1.25ms units */
            return false;
        }
        else if (params->latency > 2)
        { /** Number of intervals allowed to skip */
            return false;
        }
        else if (params->supervision_timeout > 700)
        { /** 10ms units */
            return false;
        }

        return true;
    }; 
};

/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;
static const char *_web3e_hexStr = "0123456789ABCDEF";

__attribute__((weak)) std::string ConvertBytesToHex(const uint8_t *bytes, int length)
{
    char *buffer = (char *)__builtin_alloca(length * 2 + 3);
    char *pout = buffer;
    *pout++ = '0';
    *pout++ = 'x';
    for (int i = 0; i < length; i++)
    {
        *pout++ = _web3e_hexStr[((bytes)[i] >> 4) & 0xF];
        *pout++ = _web3e_hexStr[(bytes)[i] & 0xF];
    }
    *pout = 0;
    return std::string(buffer);
}

__attribute__((weak)) std::vector<char> HexToBytes(const std::string &hex)
{
    std::vector<char> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}