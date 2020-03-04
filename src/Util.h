// Web3E Utilities
//
// By James Brown Githubs: @JamesSmartCell @AlphaWallet 
// Twitters: @TallyDigital @AlphaWallet
//
// Based on Web3 Arduino by Okada, Takahiro.
//
//

#ifndef WEB3_UTIL_H
#define WEB3_UTIL_H

#include <stdint.h>
#include <vector>
#include <sstream>
#include "uint256/uint256_t.h"

using namespace std;

class Util {
public:
    // RLP implementation
    // reference:
    //     https://github.com/ethereum/wiki/wiki/%5BEnglish%5D-RLP
    static uint32_t        RlpEncodeWholeHeader(uint8_t *header_output, uint32_t total_len);
    static vector<uint8_t> RlpEncodeWholeHeaderWithVector(uint32_t total_len);
    static uint32_t        RlpEncodeItem(uint8_t* output, const uint8_t* input, uint32_t input_len);
    static vector<uint8_t> RlpEncodeItemWithVector(const vector<uint8_t> input);

    static uint32_t        ConvertNumberToUintArray(uint8_t *str, uint32_t val);
    static vector<uint8_t> ConvertNumberToVector(uint32_t val);
    static vector<uint8_t> ConvertNumberToVector(unsigned long long val);
    static uint32_t        ConvertCharStrToUintArray(uint8_t *out, const uint8_t *in);
    static vector<uint8_t> ConvertHexToVector(const uint8_t *in);
    static vector<uint8_t> ConvertHexToVector(const string* str);
    static char *          ConvertToString(const uint8_t *in);

    static uint8_t HexToInt(uint8_t s);
    static string  VectorToString(const vector<uint8_t> *buf);
    static string  PlainVectorToString(const vector<uint8_t> *buf);
    static string  ConvertBytesToHex(const uint8_t *bytes, int length);
    static void    ConvertHexToBytes(uint8_t *_dst, const char *_src, int length);
    static string  ConvertBase(int from, int to, const char *s);
    static string  ConvertDecimal(int decimal, string *s);
    static string  ConvertString(const char* value);
    static string  ConvertHexToASCII(const char *result, size_t length);
    static string  InterpretStringResult(const char *result);
    static vector<string>* InterpretVectorResult(string *result);
    static void PadForward(string *target, int targetSize);
    static uint256_t ConvertToWei(double val, int decimals);
    static string ConvertWeiToEthString(uint256_t *weiVal, int decimals);

    static vector<string>* ConvertCharStrToVector32(const char *resultPtr, size_t resultSize, vector<string> *result);

    static string  ConvertEthToWei(double eth);
    static string toString(int value);

    static string ConvertIntegerToBytes(const int32_t value);

private:
    static uint8_t ConvertCharToByte(const uint8_t* ptr);

};

#endif //WEB3_UTIL_H
