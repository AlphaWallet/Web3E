//
// Created by Okada, Takahiro on 2018/02/11.
//

#ifndef WEB3_UTIL_H
#define WEB3_UTIL_H

#include <WiFi.h>
#include <stdint.h>
#include <vector>

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
    static uint32_t        ConvertCharStrToUintArray(uint8_t *out, const uint8_t *in);
    static vector<uint8_t> ConvertCharStrToVector(const uint8_t *in);
    static vector<uint8_t> ConvertStringToVector(const string* str);

    static uint8_t HexToInt(uint8_t s);
    static void    BufToCharStr(char* str, const uint8_t* buf, uint32_t len);
    static void    VectorToCharStr(char* str, const vector<uint8_t> buf);
    static string  VectorToString(const vector<uint8_t> buf);
    static String  BytesToHex(uint8_t *bytes, int length);
    static void    ConvertToBytes(uint8_t *_dst, const char *_src, int length);
};

#endif //WEB3_UTIL_H
