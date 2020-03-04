// Web3E Contract handling code
//
// By James Brown Githubs: @JamesSmartCell @AlphaWallet
// Twitters: @TallyDigital @AlphaWallet
//
// Based on Web3 Arduino by Okada, Takahiro.
//
//

#include "Contract.h"
#include "Web3.h"
#include <WiFi.h>
#include "Util.h"
#include "cJSON/cJSON.h"
#include <vector>

#define SIGNATURE_LENGTH 64

/**
 * Public functions
 * */

Contract::Contract(Web3* _web3, const char* address) {
    web3 = _web3;
    contractAddress = address;
    options.gas=0;
    strcpy(options.from,"");
    strcpy(options.to,"");
    strcpy(options.gasPrice,"0");
    crypto = NULL;
}

void Contract::SetPrivateKey(const char *key) {
    crypto = new Crypto(web3);
    crypto->SetPrivateKey(key);
}

string Contract::SetupContractData(const char* func, ...)
{
    string ret = "";

    string contractBytes = GenerateContractBytes(func);
    ret = contractBytes;

    size_t paramCount = 0;
    vector<string> params;
    char *p;
    char tmp[strlen(func)];
    memset(tmp, 0, sizeof(tmp));
    strcpy(tmp, func);
    strtok(tmp, "(");
    p = strtok(0, "(");
    p = strtok(p, ")");
    p = strtok(p, ",");
    if (p != 0) {
        params.push_back(string(p));
        paramCount++;
    }
    while(p != 0) {
        p = strtok(0, ",");
        if (p != 0)
        {
            params.push_back(string(p));
            paramCount++;
        }
    }

    va_list args;
    va_start(args, func);
    for( int i = 0; i < paramCount; ++i ) {
        if (strstr(params[i].c_str(), "uint") != NULL && strstr(params[i].c_str(), "[]") != NULL)
        {
            // value array
            string output = GenerateBytesForUIntArray(va_arg(args, vector<uint32_t> *));
            ret = ret + output;
        }
        else if (strncmp(params[i].c_str(), "uint", sizeof("uint")) == 0 || strncmp(params[i].c_str(), "uint256", sizeof("uint256")) == 0)
        {
            string output = GenerateBytesForUint(va_arg(args, uint256_t *));
            ret = ret + output;
        }
        else if (strncmp(params[i].c_str(), "int", sizeof("int")) == 0 || strncmp(params[i].c_str(), "bool", sizeof("bool")) == 0)
        {
            string output = GenerateBytesForInt(va_arg(args, int32_t));
            ret = ret + string(output);
        }
        else if (strncmp(params[i].c_str(), "address", sizeof("address")) == 0)
        {
            string output = GenerateBytesForAddress(va_arg(args, string *));
            ret = ret + string(output);
        }
        else if (strncmp(params[i].c_str(), "string", sizeof("string")) == 0)
        {
            string output = GenerateBytesForString(va_arg(args, string *));
            ret = ret + string(output);
        }
        else if (strncmp(params[i].c_str(), "bytes", sizeof("bytes")) == 0)
        {
            long len = strtol(params[i].c_str() + 5, nullptr, 10);
            string output = GenerateBytesForBytes(va_arg(args, char *), len);
            ret = ret + string(output);
        }
    }
    va_end(args);

    return ret;
}

string Contract::ViewCall(const string *param)
{
    string result = web3->EthViewCall(param, contractAddress);
    return result;
}

string Contract::Call(const string *param)
{
    const string from = string(options.from);
    const long gasPrice = strtol(options.gasPrice, nullptr, 10);
    const string value = "";

    string result = web3->EthCall(&from, contractAddress, options.gas, gasPrice, &value, param);
    return result;
}

string Contract::SendTransaction(uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
                                 string *toStr, uint256_t *valueStr, string *dataStr)
{
    uint8_t signature[SIGNATURE_LENGTH];
    memset(signature, 0, SIGNATURE_LENGTH);
    int recid[1] = {0};
    GenerateSignature(signature, recid, nonceVal, gasPriceVal, gasLimitVal,
                      toStr, valueStr, dataStr);

    vector<uint8_t> param = RlpEncodeForRawTransaction(nonceVal, gasPriceVal, gasLimitVal,
                                                       toStr, valueStr, dataStr,
                                                       signature, recid[0]);

    string paramStr = Util::VectorToString(&param);
    return web3->EthSendSignedTransaction(&paramStr, param.size());
}

/**
 * Private functions
 **/

void Contract::GenerateSignature(uint8_t *signature, int *recid, uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
                                 string *toStr, uint256_t *valueStr, string *dataStr)
{
    vector<uint8_t> encoded = RlpEncode(nonceVal, gasPriceVal, gasLimitVal, toStr, valueStr, dataStr);
    // hash
    string t = Util::VectorToString(&encoded);

    uint8_t *hash = new uint8_t[ETHERS_KECCAK256_LENGTH];
    size_t encodedTxBytesLength = (t.length()-2)/2;
    uint8_t *bytes = new uint8_t[encodedTxBytesLength];
    Util::ConvertHexToBytes(bytes, t.c_str(), encodedTxBytesLength);

    Crypto::Keccak256((uint8_t*)bytes, encodedTxBytesLength, hash);

    // sign
    Sign((uint8_t *)hash, signature, recid);
}

std::string Contract::GenerateContractBytes(const char *func)
{
    std::string in = Util::ConvertBytesToHex((const uint8_t *)func, strlen(func));
    //get the hash of the input
    std::vector<uint8_t> contractBytes = Util::ConvertHexToVector(&in);
    std::string out = Crypto::Keccak256(&contractBytes);
    out.resize(10);
    return out;
}

string Contract::GenerateBytesForUint(const uint256_t *value)
{
    std::vector<uint8_t> bits = value->export_bits();
    return Util::PlainVectorToString(&bits);
}

string Contract::GenerateBytesForInt(const int32_t value)
{
    return string(56, '0') + Util::ConvertIntegerToBytes(value);
}

string Contract::GenerateBytesForUIntArray(const vector<uint32_t> *v)
{
    string dynamicMarker = std::string(64, '0');
    dynamicMarker.at(62) = '4'; //0x000...40 Array Designator
    string arraySize = GenerateBytesForInt(v->size());
    string output = dynamicMarker + arraySize;
    for (auto itr = v->begin(); itr != v->end(); itr++)
    {
        output += GenerateBytesForInt(*itr);
    }

    return output;
}

string Contract::GenerateBytesForAddress(const string *v)
{
    string cleaned = *v;
    if (v->at(0) == 'x') cleaned = v->substr(1);
    else if (v->at(1) == 'x') cleaned = v->substr(2);
    size_t digits = cleaned.length();
    return string(64 - digits, '0') + cleaned;
}

string Contract::GenerateBytesForString(const string *value)
{
    const char *valuePtr = value->c_str(); //don't fail if given a 'String'
    size_t length = strlen(valuePtr);
    return GenerateBytesForBytes(valuePtr, length);
}

string Contract::GenerateBytesForBytes(const char *value, const int len)
{
    string bytesStr = Util::ConvertBytesToHex((const uint8_t *)value, len);
    size_t digits = bytesStr.length();
    return bytesStr + string(64 - digits, '0');
}

vector<uint8_t> Contract::RlpEncode(
    uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
    string *toStr, uint256_t *val, string *dataStr)
{
    vector<uint8_t> nonce = Util::ConvertNumberToVector(nonceVal);
    vector<uint8_t> gasPrice = Util::ConvertNumberToVector(gasPriceVal);
    vector<uint8_t> gasLimit = Util::ConvertNumberToVector(gasLimitVal);
    vector<uint8_t> to = Util::ConvertHexToVector(toStr);
    vector<uint8_t> value = val->export_bits_truncate();
    vector<uint8_t> data = Util::ConvertHexToVector(dataStr);

    vector<uint8_t> outputNonce = Util::RlpEncodeItemWithVector(nonce);
    vector<uint8_t> outputGasPrice = Util::RlpEncodeItemWithVector(gasPrice);
    vector<uint8_t> outputGasLimit = Util::RlpEncodeItemWithVector(gasLimit);
    vector<uint8_t> outputTo = Util::RlpEncodeItemWithVector(to);
    vector<uint8_t> outputValue = Util::RlpEncodeItemWithVector(value);
    vector<uint8_t> outputData = Util::RlpEncodeItemWithVector(data);

    vector<uint8_t> encoded = Util::RlpEncodeWholeHeaderWithVector(
        outputNonce.size() +
        outputGasPrice.size() +
        outputGasLimit.size() +
        outputTo.size() +
        outputValue.size() +
        outputData.size());

    encoded.insert(encoded.end(), outputNonce.begin(), outputNonce.end());
    encoded.insert(encoded.end(), outputGasPrice.begin(), outputGasPrice.end());
    encoded.insert(encoded.end(), outputGasLimit.begin(), outputGasLimit.end());
    encoded.insert(encoded.end(), outputTo.begin(), outputTo.end());
    encoded.insert(encoded.end(), outputValue.begin(), outputValue.end());
    encoded.insert(encoded.end(), outputData.begin(), outputData.end());

    return encoded;
}

void Contract::Sign(uint8_t *hash, uint8_t *sig, int *recid)
{
    BYTE fullSig[65];
    crypto->Sign(hash, fullSig);
    *recid = fullSig[64];
    memcpy(sig,fullSig, 64);
}

vector<uint8_t> Contract::RlpEncodeForRawTransaction(
    uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
    string *toStr, uint256_t *val, string *dataStr, uint8_t *sig, uint8_t recid)
{

    vector<uint8_t> signature;
    for (int i = 0; i < SIGNATURE_LENGTH; i++)
    {
        signature.push_back(sig[i]);
    }
    vector<uint8_t> nonce = Util::ConvertNumberToVector(nonceVal);
    vector<uint8_t> gasPrice = Util::ConvertNumberToVector(gasPriceVal);
    vector<uint8_t> gasLimit = Util::ConvertNumberToVector(gasLimitVal);
    vector<uint8_t> to = Util::ConvertHexToVector(toStr);
    vector<uint8_t> value = val->export_bits_truncate();
    vector<uint8_t> data = Util::ConvertHexToVector(dataStr);

    vector<uint8_t> outputNonce = Util::RlpEncodeItemWithVector(nonce);
    vector<uint8_t> outputGasPrice = Util::RlpEncodeItemWithVector(gasPrice);
    vector<uint8_t> outputGasLimit = Util::RlpEncodeItemWithVector(gasLimit);
    vector<uint8_t> outputTo = Util::RlpEncodeItemWithVector(to);
    vector<uint8_t> outputValue = Util::RlpEncodeItemWithVector(value);
    vector<uint8_t> outputData = Util::RlpEncodeItemWithVector(data);

    vector<uint8_t> R;
    R.insert(R.end(), signature.begin(), signature.begin()+(SIGNATURE_LENGTH/2));
    vector<uint8_t> S;
    S.insert(S.end(), signature.begin()+(SIGNATURE_LENGTH/2), signature.end());
    vector<uint8_t> V;
    V.push_back((uint8_t)(recid+27)); // 27 is a magic number for Ethereum spec
    vector<uint8_t> outputR = Util::RlpEncodeItemWithVector(R);
    vector<uint8_t> outputS = Util::RlpEncodeItemWithVector(S);
    vector<uint8_t> outputV = Util::RlpEncodeItemWithVector(V);
    vector<uint8_t> encoded = Util::RlpEncodeWholeHeaderWithVector(
        outputNonce.size() +
        outputGasPrice.size() +
        outputGasLimit.size() +
        outputTo.size() +
        outputValue.size() +
        outputData.size() +
        outputR.size() +
        outputS.size() +
        outputV.size());

    encoded.insert(encoded.end(), outputNonce.begin(), outputNonce.end());
    encoded.insert(encoded.end(), outputGasPrice.begin(), outputGasPrice.end());
    encoded.insert(encoded.end(), outputGasLimit.begin(), outputGasLimit.end());
    encoded.insert(encoded.end(), outputTo.begin(), outputTo.end());
    encoded.insert(encoded.end(), outputValue.begin(), outputValue.end());
    encoded.insert(encoded.end(), outputData.begin(), outputData.end());
    encoded.insert(encoded.end(), outputV.begin(), outputV.end());
    encoded.insert(encoded.end(), outputR.begin(), outputR.end());
    encoded.insert(encoded.end(), outputS.begin(), outputS.end());

    return encoded;
}
