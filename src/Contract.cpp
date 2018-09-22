//
//

#include "Contract.h"
#include "Web3.h"
#include <WiFi.h>
#include "Util.h"
#include "Log.h"
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
    va_start(args, paramCount);
    for( int i = 0; i < paramCount; ++i ) {
        if (strstr(params[i].c_str(), "uint") != NULL && strstr(params[i].c_str(), "[]") != NULL)
        {
            // value array
            string output = GenerateBytesForUIntArray(va_arg(args, vector<uint32_t> *));
            ret = ret + output;
        }
        else if (strncmp(params[i].c_str(), "uint", sizeof("uint")) == 0 || strncmp(params[i].c_str(), "uint256", sizeof("uint256")) == 0)
        {
            string output = GenerateBytesForUint(va_arg(args, uint32_t));
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
                                 string *toStr, string *valueStr, string *dataStr)
{
    uint8_t signature[SIGNATURE_LENGTH];
    memset(signature, 0, SIGNATURE_LENGTH);
    int recid[1] = {0};
    GenerateSignature(signature, recid, nonceVal, gasPriceVal, gasLimitVal,
                      toStr, valueStr, dataStr);

    vector<uint8_t> param = RlpEncodeForRawTransaction(nonceVal, gasPriceVal, gasLimitVal,
                                                       toStr, valueStr, dataStr,
                                                       signature, recid[0]);

#if 0
    Serial.println("RLP RawTrans Encode:");
    Serial.println(Util::ConvertBytesToHex(param.data(), param.size()).c_str());
#endif

    string paramStr = Util::VectorToString(param);
    return web3->EthSendSignedTransaction(&paramStr, param.size());
}

/**
 * Private functions
 **/

void Contract::GenerateSignature(uint8_t *signature, int *recid, uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
                                 string *toStr, string *valueStr, string *dataStr)
{
    vector<uint8_t> encoded = RlpEncode(nonceVal, gasPriceVal, gasLimitVal, toStr, valueStr, dataStr);
    // hash
    string t = Util::VectorToString(encoded);

    uint8_t *hash = new uint8_t[ETHERS_KECCAK256_LENGTH];
    size_t encodedTxBytesLength = (t.length()-2)/2;
    uint8_t *bytes = new uint8_t[encodedTxBytesLength];
    Util::ConvertHexToBytes(bytes, t.c_str(), encodedTxBytesLength);

    Crypto::Keccak256((uint8_t*)bytes, encodedTxBytesLength, hash);

#if 0
    Serial.print("Digest: ");
    Serial.println(Util::ConvertBytesToHex(hash, ETHERS_KECCAK256_LENGTH).c_str());
#endif

    // sign
    Sign((uint8_t *)hash, signature, recid);

#if 0
    Serial.print("Sig: ");
    Serial.println(Util::ConvertBytesToHex(signature, SIGNATURE_LENGTH).c_str());
#endif
}

string Contract::GenerateContractBytes(const char *func)
{
    string in = "0x";
    char intmp[8];
    memset(intmp, 0, 8);

    for (int i = 0; i < 128; i++)
    {
        char c = func[i];
        if (c == '\0')
        {
            break;
        }
        sprintf(intmp, "%x", c);
        in = in + intmp;
    }
    //get the hash of the input
    string out = Crypto::Keccak256(Util::ConvertHexToVector(&in));
    out.resize(10);
    return out;
}

string Contract::GenerateBytesForUint(const uint32_t value)
{
    char output[70];
    memset(output, 0, sizeof(output));

    // check number of digits
    char dummy[64];
    int digits = sprintf(dummy, "%x", (uint32_t)value);

    // fill 0 and copy number to string
    for (int i = 2; i < 2 + 64 - digits; i++)
    {
        sprintf(output, "%s%s", output, "0");
    }
    sprintf(output, "%s%x", output, (uint32_t)value);
    return string(output);
}

string Contract::GenerateBytesForInt(const int32_t value)
{
    char output[70];
    memset(output, 0, sizeof(output));

    // check number of digits
    char dummy[64];
    int digits = sprintf(dummy, "%x", value);

    // fill 0 and copy number to string
    char fill[2];
    if (value >= 0)
    {
        sprintf(fill, "%s", "0");
    }
    else
    {
        sprintf(fill, "%s", "f");
    }
    for (int i = 2; i < 2 + 64 - digits; i++)
    {
        sprintf(output, "%s%s", output, fill);
    }
    sprintf(output, "%s%x", output, value);
    return string(output);
}

string Contract::GenerateBytesForUIntArray(const vector<uint32_t> *v)
{
    string output;
    char numstr[21];
    string dynamicMarker = "40";
    Util::PadForward(&dynamicMarker, 32);
    string arraySize = itoa(v->size(), numstr, 16);
    Util::PadForward(&arraySize, 32);
    output = dynamicMarker + arraySize;
    for (auto itr = v->begin(); itr != v->end(); itr++)
    {
        string element = itoa(*itr, numstr, 16);
        Util::PadForward(&element, 32);
        output += element;
    }

    return output;
}

string Contract::GenerateBytesForAddress(const string *v)
{
    const char *value = v->c_str();
    size_t digits = strlen(value) - 2;

    string zeros = "";
    for (int i = 2; i < 2 + 64 - digits; i++)
    {
        zeros = zeros + "0";
    }
    return zeros + string(value + 2);
}

string Contract::GenerateBytesForString(const string *value)
{
    const char *valuePtr = value->c_str(); //don't fail if given a 'String'
    string zeros = "";
    size_t remain = 32 - ((strlen(valuePtr) - 2) % 32);
    for (int i = 0; i < remain + 32; i++)
    {
        zeros = zeros + "0";
    }

    return string(valuePtr + zeros);
}

string Contract::GenerateBytesForBytes(const char *value, const int len)
{
    char output[70];
    memset(output, 0, sizeof(output));

    for (int i = 0; i < len; i++)
    {
        sprintf(output, "%s%x", output, value[i]);
    }
    size_t remain = 32 - ((strlen(output) - 2) % 32);
    for (int i = 0; i < remain + 32; i++)
    {
        sprintf(output, "%s%s", output, "0");
    }

    return string(output);
}

vector<uint8_t> Contract::RlpEncode(
    uint32_t nonceVal, unsigned long long gasPriceVal, uint32_t gasLimitVal,
    string *toStr, string *valueStr, string *dataStr)
{
    vector<uint8_t> nonce = Util::ConvertNumberToVector(nonceVal);
    vector<uint8_t> gasPrice = Util::ConvertNumberToVector(gasPriceVal);
    vector<uint8_t> gasLimit = Util::ConvertNumberToVector(gasLimitVal);
    vector<uint8_t> to = Util::ConvertHexToVector(toStr);
    vector<uint8_t> value = Util::ConvertHexToVector(valueStr);
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

#if 0
    Serial.println("RLP Encode:");
    Serial.println(Util::ConvertBytesToHex(encoded.data(), encoded.size()).c_str());
#endif

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
    string *toStr, string *valueStr, string *dataStr, uint8_t *sig, uint8_t recid)
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
    vector<uint8_t> value = Util::ConvertHexToVector(valueStr);
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

#if 0
    printf("\noutputNonce--------\n ");
    for (int i = 0; i<outputNonce.size(); i++) { printf("%02x ", outputNonce[i]); }
    printf("\noutputGasPrice--------\n ");
    for (int i = 0; i<outputGasPrice.size(); i++) {printf("%02x ", outputGasPrice[i]); }
    printf("\noutputGasLimit--------\n ");
    for (int i = 0; i<outputGasLimit.size(); i++) {printf("%02x ", outputGasLimit[i]); }
    printf("\noutputTo--------\n ");
    for (int i = 0; i<outputTo.size(); i++) {printf("%02x ", outputTo[i]); }
    printf("\noutputValue--------\n ");
    for (int i = 0; i<outputValue.size(); i++) { printf("%02x ", outputValue[i]); }
    printf("\noutputData--------\n ");
    for (int i = 0; i<outputData.size(); i++) { printf("%02x ", outputData[i]); }
    printf("\nR--------\n ");
    for (int i = 0; i<outputR.size(); i++) { printf("%02x ", outputR[i]); }
    printf("\nS--------\n ");
    for (int i = 0; i<outputS.size(); i++) { printf("%02x ", outputS[i]); }
    printf("\nV--------\n ");
    for (int i = 0; i<outputV.size(); i++) { printf("%02x ", outputV[i]); }
    printf("\n");
#endif

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
