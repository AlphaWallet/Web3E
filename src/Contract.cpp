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

Contract::Contract(long long networkId) : Contract(new Web3(networkId), "") {}

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

    vector<string> abiBlocks;
    vector<bool> isDynamic;
    int dynamicStartPointer = 0;

    va_list args;
    va_start(args, func);
    for( int i = 0; i < paramCount; ++i ) {
        if (strstr(params[i].c_str(), "uint") != NULL && strstr(params[i].c_str(), "[]") != NULL)
        {
            // value array
            string output = GenerateBytesForUIntArray(va_arg(args, vector<uint32_t> *));
            abiBlocks.push_back(output);
            isDynamic.push_back(false);
            dynamicStartPointer += 0x20;
        }
        else if (strncmp(params[i].c_str(), "uint", sizeof("uint")) == 0 || strncmp(params[i].c_str(), "uint256", sizeof("uint256")) == 0)
        {
            string output = GenerateBytesForUint(va_arg(args, uint256_t *));
            abiBlocks.push_back(output);
            isDynamic.push_back(false);
            dynamicStartPointer += 0x20;
        }
        else if (strncmp(params[i].c_str(), "int", sizeof("int")) == 0 || strncmp(params[i].c_str(), "bool", sizeof("bool")) == 0)
        {
            string output = GenerateBytesForInt(va_arg(args, int32_t));
            abiBlocks.push_back(output);
            isDynamic.push_back(false);
            dynamicStartPointer += 0x20;
        }
        else if (strncmp(params[i].c_str(), "address", sizeof("address")) == 0)
        {
            string output = GenerateBytesForAddress(va_arg(args, string *));
            abiBlocks.push_back(output);
            isDynamic.push_back(false);
            dynamicStartPointer += 0x20;
        }
        else if (strncmp(params[i].c_str(), "string", sizeof("string")) == 0)
        {
            string output = GenerateBytesForString(va_arg(args, string *));
            abiBlocks.push_back(output);
            isDynamic.push_back(true);
            dynamicStartPointer += 0x20;
        }
        else if (strncmp(params[i].c_str(), "bytes", sizeof("bytes")) == 0) //if sending bytes, take the value in hex
        {
            string output = GenerateBytesForHexBytes(va_arg(args, string *));
            abiBlocks.push_back(output);
            isDynamic.push_back(true);
            dynamicStartPointer += 0x20;
        }
        else if (strncmp(params[i].c_str(), "struct", sizeof("struct")) == 0) //if sending bytes, take the value in hex
        {
            string output = GenerateBytesForStruct(va_arg(args, string *));
            abiBlocks.push_back(output);
            isDynamic.push_back(true);
            dynamicStartPointer += 0x20;
        }
    }
    va_end(args);

    uint256_t abiOffet = uint256_t(dynamicStartPointer);
    //now build output - parse 1, standard params
    for( int i = 0; i < paramCount; ++i ) 
    {
        if (isDynamic[i])
        {
            ret = ret + abiOffet.str(16, 64);
            string *outputHex = &abiBlocks[i];
            abiOffet += outputHex->size() / 2;
        }
        else
        {
            ret = ret + abiBlocks[i];
        }
    }

    //parse 2: add dynamic params
    for( int i = 0; i < paramCount; ++i ) 
    {
        if (isDynamic[i])
        {
            ret = ret + abiBlocks[i];
        }
    }

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

string
Contract::SignTransaction(uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal, string *toStr,
                          uint256_t *valueStr, string *dataStr) {

    uint8_t signature[SIGNATURE_LENGTH];
    memset(signature, 0, SIGNATURE_LENGTH);
    int recid[1] = {0};

    GenerateSignature(signature, recid, nonceVal, gasPriceVal, gasLimitVal,
                      toStr, valueStr, dataStr);

    vector<uint8_t> param = RlpEncodeForRawTransaction(nonceVal, gasPriceVal, gasLimitVal,
                                                       toStr, valueStr, dataStr,
                                                       signature, recid[0]);
    return Util::VectorToString(&param);
}

/**
 * Utility functions
 **/

void Contract::ReplaceFunction(std::string &param, const char* func)
{
    param = GenerateContractBytes(func) + param.substr(10);
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

string Contract::GenerateBytesForHexBytes(const string *value)
{
    string cleaned = *value;
    if (value->at(0) == 'x') cleaned = value->substr(1);
    else if (value->at(1) == 'x') cleaned = value->substr(2);
    string digitsStr = Util::intToHex(cleaned.length() / 2); //bytes length will be hex length / 2
    string lengthDesignator = string(64 - digitsStr.length(), '0') + digitsStr;
    cleaned = lengthDesignator + cleaned;
    size_t digits = cleaned.length() % 64;
    return cleaned + (digits > 0 ? string(64 - digits, '0') : "");
}

string Contract::GenerateBytesForStruct(const string *value)
{
    //struct has no length params: not required
    string cleaned = *value;
    if (value->at(0) == 'x') cleaned = value->substr(1);
    else if (value->at(1) == 'x') cleaned = value->substr(2);
    size_t digits = cleaned.length() % 64;
    return cleaned + (digits > 0 ? string(64 - digits, '0') : "");
}

string Contract::GenerateBytesForBytes(const char *value, const int len)
{
    string bytesStr = Util::ConvertBytesToHex((const uint8_t *)value, len).substr(2); //clean hex prefix;
    size_t digits = bytesStr.length() % 64;
    return bytesStr + (digits > 0 ? string(64 - digits, '0') : "");
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
    vector<uint8_t> chainId = Util::ConvertNumberToVector(uint64_t(web3->getChainId()));

    auto *zeroStr = new string("0");
    vector<uint8_t> zero = Util::ConvertHexToVector(zeroStr);
    vector<uint8_t> outputNonce = Util::RlpEncodeItemWithVector(nonce);
    vector<uint8_t> outputGasPrice = Util::RlpEncodeItemWithVector(gasPrice);
    vector<uint8_t> outputGasLimit = Util::RlpEncodeItemWithVector(gasLimit);
    vector<uint8_t> outputTo = Util::RlpEncodeItemWithVector(to);
    vector<uint8_t> outputValue = Util::RlpEncodeItemWithVector(value);
    vector<uint8_t> outputData = Util::RlpEncodeItemWithVector(data);

    vector<uint8_t> outputChainId = Util::RlpEncodeItemWithVector(chainId);
    vector<uint8_t> outputZero = Util::RlpEncodeItemWithVector(zero);

    vector<uint8_t> encoded = Util::RlpEncodeWholeHeaderWithVector(
        outputNonce.size() +
        outputGasPrice.size() +
        outputGasLimit.size() +
        outputTo.size() +
        outputValue.size() +
        outputData.size() +
        outputChainId.size() +
        outputZero.size() +
        outputZero.size());

    encoded.insert(encoded.end(), outputNonce.begin(), outputNonce.end());
    encoded.insert(encoded.end(), outputGasPrice.begin(), outputGasPrice.end());
    encoded.insert(encoded.end(), outputGasLimit.begin(), outputGasLimit.end());
    encoded.insert(encoded.end(), outputTo.begin(), outputTo.end());
    encoded.insert(encoded.end(), outputValue.begin(), outputValue.end());
    encoded.insert(encoded.end(), outputData.begin(), outputData.end());

    encoded.insert(encoded.end(), outputChainId.begin(), outputChainId.end());
    encoded.insert(encoded.end(), outputZero.begin(), outputZero.end());
    encoded.insert(encoded.end(), outputZero.begin(), outputZero.end());

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
    //V.push_back((uint8_t)(recid + web3->getChainId() * 2 + 35)); // according to EIP-155
    uint256_t vv = recid + (web3->getChainId() * 2) + 35; // EIP-155 ensure long chainIds work correctly
    vector<uint8_t> V = vv.export_bits_truncate(); //convert to bytes
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
