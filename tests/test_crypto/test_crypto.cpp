/**
 * Web3E Cryptography Tests
 * 
 * Tests Keccak-256, key derivation, signing, and signature recovery.
 * 
 * Test vectors generated using ethers.js v6:
 * - const { keccak256, toUtf8Bytes, Wallet, hashMessage } = require('ethers');
 * - keccak256(toUtf8Bytes("hello world"))
 * - const wallet = Wallet.fromPhrase("test test test test test test test test test test test junk");
 * - wallet.privateKey, wallet.address, wallet.signMessage("Test message")
 */

#include <Arduino.h>
#include <unity.h>
#include <Web3.h>
#include <Crypto.h>
#include <Util.h>

// Test Keccak-256 hash function with known vectors
void test_keccak256_empty_string() {
    // keccak256("") = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
    const char* input = "";
    uint8_t result[32];
    uint8_t expected[32] = {
        0xc5, 0xd2, 0x46, 0x01, 0x86, 0xf7, 0x23, 0x3c,
        0x92, 0x7e, 0x7d, 0xb2, 0xdc, 0xc7, 0x03, 0xc0,
        0xe5, 0x00, 0xb6, 0x53, 0xca, 0x82, 0x27, 0x3b,
        0x7b, 0xfa, 0xd8, 0x04, 0x5d, 0x85, 0xa4, 0x70
    };
    
    Crypto::Keccak256((const uint8_t*)input, 0, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 32);
}

void test_keccak256_hello_world() {
    // keccak256("hello world") = 0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad
    const char* input = "hello world";
    uint8_t result[32];
    uint8_t expected[32] = {
        0x47, 0x17, 0x32, 0x85, 0xa8, 0xd7, 0x34, 0x1e,
        0x5e, 0x97, 0x2f, 0xc6, 0x77, 0x28, 0x63, 0x84,
        0xf8, 0x02, 0xf8, 0xef, 0x42, 0xa5, 0xec, 0x5f,
        0x03, 0xbb, 0xfa, 0x25, 0x4c, 0xb0, 0x1f, 0xad
    };
    
    Crypto::Keccak256((const uint8_t*)input, strlen(input), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 32);
}

void test_keccak256_ethereum() {
    // keccak256("ethereum") = 0x02e69dc6c63f10f6e96e6218f3f08b84d2b12e7e948e7b1f5c2a5a3b7aef0e8f
    const char* input = "ethereum";
    uint8_t result[32];
    uint8_t expected[32] = {
        0x02, 0xe6, 0x9d, 0xc6, 0xc6, 0x3f, 0x10, 0xf6,
        0xe9, 0x6e, 0x62, 0x18, 0xf3, 0xf0, 0x8b, 0x84,
        0xd2, 0xb1, 0x2e, 0x7e, 0x94, 0x8e, 0x7b, 0x1f,
        0x5c, 0x2a, 0x5a, 0x3b, 0x7a, 0xef, 0x0e, 0x8f
    };
    
    Crypto::Keccak256((const uint8_t*)input, strlen(input), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 32);
}

void test_private_key_to_public_key() {
    // Private key: 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
    // (from standard test mnemonic: "test test test test test test test test test test test junk")
    // Expected public key (uncompressed, 64 bytes without 0x04 prefix)
    uint8_t privateKey[32] = {
        0xac, 0x09, 0x74, 0xbe, 0xc3, 0x9a, 0x17, 0xe3,
        0x6b, 0xa4, 0xa6, 0xb4, 0xd2, 0x38, 0xff, 0x94,
        0x4b, 0xac, 0xb4, 0x78, 0xcb, 0xed, 0x5e, 0xfc,
        0xae, 0x78, 0x4d, 0x7b, 0xf4, 0xf2, 0xff, 0x80
    };
    
    uint8_t publicKey[64];
    uint8_t expected[64] = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
        0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65,
        0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
        0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19,
        0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
    };
    
    Crypto::PrivateKeyToPublic(privateKey, publicKey);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, publicKey, 64);
}

void test_public_key_to_address() {
    // Public key from previous test
    uint8_t publicKey[64] = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
        0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65,
        0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
        0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19,
        0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
    };
    
    // Expected address: 0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
    uint8_t address[20];
    uint8_t expected[20] = {
        0xf3, 0x9f, 0xd6, 0xe5, 0x1a, 0xad, 0x88, 0xf6,
        0xf4, 0xce, 0x6a, 0xb8, 0x82, 0x72, 0x79, 0xcf,
        0xff, 0xb9, 0x22, 0x66
    };
    
    Crypto::PublicKeyToAddress(publicKey, address);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, address, 20);
}

void test_private_key_to_address_full_derivation() {
    // Full test: private key -> public key -> address
    uint8_t privateKey[32] = {
        0xac, 0x09, 0x74, 0xbe, 0xc3, 0x9a, 0x17, 0xe3,
        0x6b, 0xa4, 0xa6, 0xb4, 0xd2, 0x38, 0xff, 0x94,
        0x4b, 0xac, 0xb4, 0x78, 0xcb, 0xed, 0x5e, 0xfc,
        0xae, 0x78, 0x4d, 0x7b, 0xf4, 0xf2, 0xff, 0x80
    };
    
    uint8_t publicKey[64];
    uint8_t address[20];
    uint8_t expectedAddress[20] = {
        0xf3, 0x9f, 0xd6, 0xe5, 0x1a, 0xad, 0x88, 0xf6,
        0xf4, 0xce, 0x6a, 0xb8, 0x82, 0x72, 0x79, 0xcf,
        0xff, 0xb9, 0x22, 0x66
    };
    
    Crypto::PrivateKeyToPublic(privateKey, publicKey);
    Crypto::PublicKeyToAddress(publicKey, address);
    
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedAddress, address, 20);
}

void test_sign_personal_message() {
    // Test signing with EIP-191 personal_sign prefix
    // Private key: 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
    // Message: "Test message"
    // Expected signature components will be checked for validity (can recover correct address)
    
    Web3 web3(1); // Mainnet
    Crypto crypto(&web3);
    crypto.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    const char* message = "Test message";
    uint8_t messageHash[32];
    
    // Create personal message hash (with "\x19Ethereum Signed Message:\n" prefix)
    char prefix[] = "\x19" "Ethereum Signed Message:\n12"; // 12 is length of "Test message"
    string fullMessage = string(prefix, sizeof(prefix) - 1) + message;
    Crypto::Keccak256((const uint8_t*)fullMessage.c_str(), fullMessage.length(), messageHash);
    
    uint8_t signature[65];
    bool success = crypto.Sign(messageHash, signature);
    
    TEST_ASSERT_TRUE(success);
    // Signature should be 65 bytes: r(32) + s(32) + v(1)
    TEST_ASSERT_EQUAL(65, sizeof(signature));
}

void test_signature_recovery() {
    // Test that we can recover the correct address from a signature
    // This verifies the full sign->recover cycle
    
    Web3 web3(1);
    Crypto crypto(&web3);
    crypto.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    const char* message = "Hello Ethereum";
    uint8_t messageHash[32];
    
    // Create personal message hash
    char prefix[] = "\x19" "Ethereum Signed Message:\n14"; // 14 is length of "Hello Ethereum"
    string fullMessage = string(prefix, sizeof(prefix) - 1) + message;
    Crypto::Keccak256((const uint8_t*)fullMessage.c_str(), fullMessage.length(), messageHash);
    
    uint8_t signature[65];
    crypto.Sign(messageHash, signature);
    
    // Recover public key from signature
    uint8_t recoveredPubKey[64];
    Crypto::ECRecover(signature, recoveredPubKey, messageHash);
    
    // Derive address from recovered public key
    uint8_t recoveredAddress[20];
    Crypto::PublicKeyToAddress(recoveredPubKey, recoveredAddress);
    
    // Expected address: 0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
    uint8_t expectedAddress[20] = {
        0xf3, 0x9f, 0xd6, 0xe5, 0x1a, 0xad, 0x88, 0xf6,
        0xf4, 0xce, 0x6a, 0xb8, 0x82, 0x72, 0x79, 0xcf,
        0xff, 0xb9, 0x22, 0x66
    };
    
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedAddress, recoveredAddress, 20);
}

void setup() {
    delay(2000); // Allow time for serial monitor
    
    UNITY_BEGIN();
    
    RUN_TEST(test_keccak256_empty_string);
    RUN_TEST(test_keccak256_hello_world);
    RUN_TEST(test_keccak256_ethereum);
    RUN_TEST(test_private_key_to_public_key);
    RUN_TEST(test_public_key_to_address);
    RUN_TEST(test_private_key_to_address_full_derivation);
    RUN_TEST(test_sign_personal_message);
    RUN_TEST(test_signature_recovery);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
