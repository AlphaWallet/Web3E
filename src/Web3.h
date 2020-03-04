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

class Web3;
class Crypto;
class KeyID;

#include "stdint.h"
#include "uint256/uint256_t.h"
#include <Contract.h>
#include <Crypto.h>
#include <KeyID.h>
#include <ScriptClient.h>
#include <UdpBridge.h>
#include <Util.h>
#include <string.h>
#include <string>

using namespace std;

class Web3 {
public:
    Web3(const char* _host, const char* _path);
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

    static const char* getDAppCode();

    long long int getLongLong(const string* json);
    string getString(const string* json);
    int getInt(const string* json);
    uint256_t getUint256(const string* json);

private:
    string exec(const string* data);
    string generateJson(const string* method, const string* params);
    
    long getLong(const string* json);
    double getDouble(const string* json);
    bool getBool(const string* json);

private:
    const char* host;
    const char* path;
};

#endif //ARDUINO_WEB3_WEB3_H
