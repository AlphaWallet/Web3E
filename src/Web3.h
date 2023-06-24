// Web3E main header
//
// By James Brown Githubs: @JamesSmartCell @AlphaWallet 
// Twitters: @TallyDigital @AlphaWallet
//
// Based on Web3 Arduino by Okada, Takahiro.
//
//

#ifndef ARDUINO_WEB3_WEB3_H
#define ARDUINO_WEB3_WEB3_H

typedef unsigned char BYTE;
#define ETHERS_PRIVATEKEY_LENGTH       32
#define ETHERS_PUBLICKEY_LENGTH        64
#define ETHERS_ADDRESS_LENGTH          20
#define ETHERS_KECCAK256_LENGTH        32
#define ETHERS_SIGNATURE_LENGTH        65

#define INFURA_API_KEY "a9d6b1764a464faaa8f0399958601361" //For production you will want to make your own API key, visit https://infura.io

class Web3;
class Crypto;
class KeyID;

#include "stdint.h"
#include "uint256/uint256_t.h"
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <Contract.h>
#include <Crypto.h>
#include <KeyID.h>
#include <Util.h>
#include <string.h>
#include <string>
#include "chainIds.h"

using namespace std;

enum ConnectionStage
{
    unconnected,
    handshake,
    have_token,
    confirmed
};

class Web3 {
public:
    Web3(long long chainId);
    Web3(long long chainId, const char* infura_key);
    string Web3ClientVersion();
    string Web3Sha3(const string* data);
    int NetVersion();
    bool NetListening();
    int NetPeerCount();
    double EthProtocolVersion();
    bool EthSyncing();
    bool EthMining();
    double EthHashrate();
    long long int EthGasPrice();
    void EthAccounts(char** array, int size);
    int EthBlockNumber();
    uint256_t EthGetBalance(const string* address);
    int EthGetTransactionCount(const string* address);
    string EthViewCall(const string* data, const char* to);

    string EthCall(const string* from, const char* to, long gas, long gasPrice, const string* value, const string* data);
    string EthSendSignedTransaction(const string* data, const uint32_t dataLen);

    long long int getLongLong(const string* json);
    string getString(const string* json);
    int getInt(const string* json);
    uint256_t getUint256(const string* json);
    long long int getChainId() const;
    string getResult(const string* json);

private:
    string exec(const string* data);
    string generateJson(const string* method, const string* params);
    void selectHost();
    void setupCert();
    
    long getLong(const string* json);
    double getDouble(const string* json);
    bool getBool(const string* json);

private:
    WiFiClientSecure *client;
    BYTE *mem;
    const char* host;
    const char* path;
    const char* infura_key;
    unsigned short port;
    long long chainId;
};

#endif //ARDUINO_WEB3_WEB3_H
