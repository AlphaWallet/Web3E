/**
 * Web3E Utility Functions Tests
 * 
 * Tests hex conversion, Wei/Eth conversion, uint256 operations, and RLP encoding.
 * 
 * Test vectors generated using:
 * - ethers.js for Wei/Eth conversions: parseEther("1.5"), formatEther(...)
 * - web3.py for RLP encoding: from rlp import encode; encode([...])
 */

#include <Arduino.h>
#include <unity.h>
#include <Util.h>
#include <Web3.h>

void test_hex_to_bytes_conversion() {
    const char* hexStr = "0x48656c6c6f"; // "Hello" in hex
    uint8_t result[5];
    uint8_t expected[5] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
    
    Util::ConvertHexToBytes(result, hexStr, 5);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 5);
}

void test_hex_to_bytes_without_prefix() {
    const char* hexStr = "deadbeef";
    uint8_t result[4];
    uint8_t expected[4] = {0xde, 0xad, 0xbe, 0xef};
    
    Util::ConvertHexToBytes(result, hexStr, 4);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 4);
}

void test_bytes_to_hex_conversion() {
    uint8_t bytes[4] = {0xde, 0xad, 0xbe, 0xef};
    string result = Util::ConvertBytesToHex(bytes, 4);
    
    // Library emits uppercase hex: "0xDEADBEEF"
    TEST_ASSERT_EQUAL_STRING("0xDEADBEEF", result.c_str());
}

void test_convert_to_wei() {
    // 1 ETH = 1,000,000,000,000,000,000 Wei
    uint256_t result = Util::ConvertToWei(1.0, 18);
    uint256_t expected("1000000000000000000");
    
    if (!(result == expected)) {
        std::string resultStr = result.str();
        std::string expectedStr = expected.str();
        char msg[250];
        snprintf(msg, 250, "Expected %s but got %s", expectedStr.c_str(), resultStr.c_str());
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(result == expected);
}

void test_convert_to_wei_fractional() {
    // 0.5 ETH = 500,000,000,000,000,000 Wei
    uint256_t result = Util::ConvertToWei(0.5, 18);
    uint256_t expected("500000000000000000");
    
    if (!(result == expected)) {
        std::string resultStr = result.str();
        std::string expectedStr = expected.str();
        char msg[250];
        snprintf(msg, 250, "Expected %s but got %s", expectedStr.c_str(), resultStr.c_str());
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(result == expected);
}

void test_convert_to_wei_with_decimals() {
    // For ERC20 with 6 decimals: 100.5 tokens = 100,500,000 base units
    uint256_t result = Util::ConvertToWei(100.5, 6);
    uint256_t expected("100500000");
    
    if (!(result == expected)) {
        std::string resultStr = result.str();
        std::string expectedStr = expected.str();
        char msg[250];
        snprintf(msg, 250, "Expected %s but got %s", expectedStr.c_str(), resultStr.c_str());
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(result == expected);
}

void test_convert_wei_to_eth_string() {
    uint256_t weiValue("1500000000000000000"); // 1.5 ETH
    string result = Util::ConvertWeiToEthString(&weiValue, 18);
    
    // Should start with "1.5"
    if (result.find("1.5") != 0) {
        char msg[200];
        snprintf(msg, 200, "Expected string starting with '1.5' but got '%s'", result.c_str());
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(result.find("1.5") == 0);
}

void test_convert_wei_to_eth_string_small() {
    uint256_t weiValue("123456789012345678"); // 0.123456789012345678 ETH
    string result = Util::ConvertWeiToEthString(&weiValue, 18);
    
    // Should start with "0.12"
    if (result.find("0.12") != 0) {
        char msg[200];
        snprintf(msg, 200, "Expected string starting with '0.12' but got '%s'", result.c_str());
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(result.find("0.12") == 0);
}

void test_uint256_addition() {
    uint256_t a("1000000000000000000"); // 1 ETH
    uint256_t b("500000000000000000");  // 0.5 ETH
    uint256_t result = a + b;
    uint256_t expected("1500000000000000000"); // 1.5 ETH
    
    TEST_ASSERT_TRUE(result == expected);
}

void test_uint256_multiplication() {
    uint256_t a("2");
    uint256_t b("1000000000000000000"); // 1 ETH
    uint256_t result = a * b;
    uint256_t expected("2000000000000000000"); // 2 ETH
    
    TEST_ASSERT_TRUE(result == expected);
}

void test_uint256_comparison() {
    uint256_t a("1000000000000000000");
    uint256_t b("500000000000000000");
    
    TEST_ASSERT_TRUE(a > b);
    TEST_ASSERT_TRUE(b < a);
    TEST_ASSERT_TRUE(a >= b);
    TEST_ASSERT_FALSE(a == b);
}

void test_uint256_from_hex() {
    // 0xde0b6b3a7640000 = 1 ETH in hex = 1000000000000000000 decimal
    uint256_t result("0xde0b6b3a7640000");
    uint256_t expected("1000000000000000000");
    
    if (!(result == expected)) {
        std::string resultStr = result.str();
        std::string expectedStr = expected.str();
        char msg[250];
        snprintf(msg, 250, "Expected %s but got %s", expectedStr.c_str(), resultStr.c_str());
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(result == expected);
}

void test_rlp_encode_small_value() {
    // RLP encoding of single byte 0x7f should be [0x7f]
    uint8_t input = 0x7f;
    uint8_t output[2];
    uint32_t len = Util::RlpEncodeItem(output, &input, 1);
    
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL_HEX8(0x7f, output[0]);
}

void test_rlp_encode_short_string() {
    // RLP encoding of "dog" (0x646f67) should be [0x83, 0x64, 0x6f, 0x67]
    uint8_t input[3] = {0x64, 0x6f, 0x67};
    uint8_t output[10];
    uint32_t len = Util::RlpEncodeItem(output, input, 3);
    
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_HEX8(0x83, output[0]); // 0x80 + 3
    TEST_ASSERT_EQUAL_HEX8(0x64, output[1]);
    TEST_ASSERT_EQUAL_HEX8(0x6f, output[2]);
    TEST_ASSERT_EQUAL_HEX8(0x67, output[3]);
}

void test_hex_string_padded() {
    // Test ConvertHexToBytes with a properly padded address
    // NOTE: ConvertHexToBytes does NOT pad - caller must provide full-length hex string
    // For 20 bytes, need 40 hex chars (+ optional 0x prefix)
    const char* addr = "0x0000000000000000000000000000000000000001";
    uint8_t result[20];
    
    Util::ConvertHexToBytes(result, addr, 20);
    
    // First 19 bytes should be 0, last byte should be 1
    for (int i = 0; i < 19; i++) {
        if (result[i] != 0x00) {
            char msg[100];
            snprintf(msg, 100, "Byte %d: expected 0x00 but got 0x%02X", i, result[i]);
            TEST_FAIL_MESSAGE(msg);
        }
    }
    TEST_ASSERT_EQUAL_HEX8(0x01, result[19]);
}

void test_convert_number_to_vector() {
    // Test conversion of numbers to byte vectors
    vector<uint8_t> result = Util::ConvertNumberToVector((uint32_t)255);
    
    TEST_ASSERT_EQUAL(1, result.size());
    TEST_ASSERT_EQUAL_HEX8(0xff, result[0]);
}

void test_convert_number_to_vector_large() {
    // 0x1234 = 4660 decimal
    vector<uint8_t> result = Util::ConvertNumberToVector((uint32_t)4660);
    
    TEST_ASSERT_EQUAL(2, result.size());
    TEST_ASSERT_EQUAL_HEX8(0x12, result[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, result[1]);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_hex_to_bytes_conversion);
    RUN_TEST(test_hex_to_bytes_without_prefix);
    RUN_TEST(test_bytes_to_hex_conversion);
    RUN_TEST(test_convert_to_wei);
    RUN_TEST(test_convert_to_wei_fractional);
    RUN_TEST(test_convert_to_wei_with_decimals);
    RUN_TEST(test_convert_wei_to_eth_string);
    RUN_TEST(test_convert_wei_to_eth_string_small);
    RUN_TEST(test_uint256_addition);
    RUN_TEST(test_uint256_multiplication);
    RUN_TEST(test_uint256_comparison);
    RUN_TEST(test_uint256_from_hex);
    RUN_TEST(test_rlp_encode_small_value);
    RUN_TEST(test_rlp_encode_short_string);
    RUN_TEST(test_hex_string_padded);
    RUN_TEST(test_convert_number_to_vector);
    RUN_TEST(test_convert_number_to_vector_large);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
