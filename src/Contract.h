// Web3E Contract handling code
//
// By James Brown Githubs: @JamesSmartCell @AlphaWallet
// Twitters: @TallyDigital @AlphaWallet
//
// Based on Web3 Arduino by Okada, Takahiro.
//
//

#ifndef ARDUINO_WEB3_CONTRACT_H
#define ARDUINO_WEB3_CONTRACT_H

#include "Arduino.h"
#include "Web3.h"
#include <vector>
#include <Crypto.h>
#include "uint256/uint256_t.h"

using namespace std;

class Contract {

public:
    typedef struct {
        char from[80];
        char to[80];
        char gasPrice[20];
        long gas;
    } Options;
    Options options;

public:
    Contract(Web3* _web3, const char* address);
    explicit Contract(long long int networkId);
    void SetPrivateKey(const char *key);
    string SetupContractData(const char* func, ...);
    string Call(const string* param);
    string ViewCall(const string *param);
    string SendTransaction(uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
                           string *toStr, uint256_t *valueStr, string *dataStr);
    string SignTransaction(uint32_t nonceVal, unsigned long long int gasPriceVal, uint32_t gasLimitVal, string *toStr,
                           uint256_t *valueStr, string *dataStr);

    static void ReplaceFunction(std::string &param, const char* func);                       
    
private:
    Web3* web3;
    const char * contractAddress;
    Crypto* crypto;

private:
    static string GenerateContractBytes(const char *func);
    string GenerateBytesForInt(const int32_t value);
    string GenerateBytesForUint(const uint256_t *value);
    string GenerateBytesForAddress(const string *value);
    string GenerateBytesForString(const string *value);
    string GenerateBytesForBytes(const char* value, const int len);
    string GenerateBytesForUIntArray(const vector<uint32_t> *v);
    string GenerateBytesForHexBytes(const string *value);
    string GenerateBytesForStruct(const string *value);

    void GenerateSignature(uint8_t* signature, int* recid, uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t  gasLimitVal,
                           string* toStr, uint256_t* valueStr, string* dataStr);
    vector<uint8_t> RlpEncode(
            uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t  gasLimitVal,
            string* toStr, uint256_t* valueStr, string* dataStr);
    vector<uint8_t> RlpEncodeForRawTransaction(
            uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t  gasLimitVal,
            string* toStr, uint256_t* valueStr, string* dataStr, uint8_t* sig, uint8_t recid);
    void Sign(uint8_t* hash, uint8_t* sig, int* recid);
};


#endif //ARDUINO_WEB3_CONTRACT_H
