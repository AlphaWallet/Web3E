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
    // keccak256("ethereum") = 0x541111248b45b7a8dc3f5579f630e74cb01456ea6ac067d3f4d793245a255155
    // Verified with pycryptodome: Crypto.Hash.keccak.new(digest_bits=256).update(b"ethereum").hexdigest()
    const char* input = "ethereum";
    uint8_t result[32];
    uint8_t expected[32] = {
        0x54, 0x11, 0x11, 0x24, 0x8b, 0x45, 0xb7, 0xa8,
        0xdc, 0x3f, 0x55, 0x79, 0xf6, 0x30, 0xe7, 0x4c,
        0xb0, 0x14, 0x56, 0xea, 0x6a, 0xc0, 0x67, 0xd3,
        0xf4, 0xd7, 0x93, 0x24, 0x5a, 0x25, 0x51, 0x55
    };
    
    Crypto::Keccak256((const uint8_t*)input, strlen(input), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 32);
}

void test_private_key_to_public_key() {
    // Private key: 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
    // (from standard test mnemonic: "test test test test test test test test test test test junk")
    // Expected public key (uncompressed, 64 bytes without 0x04 prefix)
    // Verified with eth-keys: keys.PrivateKey(bytes.fromhex("ac09...")).public_key.to_bytes()
    uint8_t privateKey[32] = {
        0xac, 0x09, 0x74, 0xbe, 0xc3, 0x9a, 0x17, 0xe3,
        0x6b, 0xa4, 0xa6, 0xb4, 0xd2, 0x38, 0xff, 0x94,
        0x4b, 0xac, 0xb4, 0x78, 0xcb, 0xed, 0x5e, 0xfc,
        0xae, 0x78, 0x4d, 0x7b, 0xf4, 0xf2, 0xff, 0x80
    };
    
    uint8_t publicKey[64];
    uint8_t expected[64] = {
        0x83, 0x18, 0x53, 0x5b, 0x54, 0x10, 0x5d, 0x4a,
        0x7a, 0xae, 0x60, 0xc0, 0x8f, 0xc4, 0x5f, 0x96,
        0x87, 0x18, 0x1b, 0x4f, 0xdf, 0xc6, 0x25, 0xbd,
        0x1a, 0x75, 0x3f, 0xa7, 0x39, 0x7f, 0xed, 0x75,
        0x35, 0x47, 0xf1, 0x1c, 0xa8, 0x69, 0x66, 0x46,
        0xf2, 0xf3, 0xac, 0xb0, 0x8e, 0x31, 0x01, 0x6a,
        0xfa, 0xc2, 0x3e, 0x63, 0x0c, 0x5d, 0x11, 0xf5,
        0x9f, 0x61, 0xfe, 0xf5, 0x7b, 0x0d, 0x2a, 0xa5
    };
    
    Crypto::PrivateKeyToPublic(privateKey, publicKey);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, publicKey, 64);
}

void test_public_key_to_address() {
    // Public key from previous test (correct public key for the test mnemonic)
    uint8_t publicKey[64] = {
        0x83, 0x18, 0x53, 0x5b, 0x54, 0x10, 0x5d, 0x4a,
        0x7a, 0xae, 0x60, 0xc0, 0x8f, 0xc4, 0x5f, 0x96,
        0x87, 0x18, 0x1b, 0x4f, 0xdf, 0xc6, 0x25, 0xbd,
        0x1a, 0x75, 0x3f, 0xa7, 0x39, 0x7f, 0xed, 0x75,
        0x35, 0x47, 0xf1, 0x1c, 0xa8, 0x69, 0x66, 0x46,
        0xf2, 0xf3, 0xac, 0xb0, 0x8e, 0x31, 0x01, 0x6a,
        0xfa, 0xc2, 0x3e, 0x63, 0x0c, 0x5d, 0x11, 0xf5,
        0x9f, 0x61, 0xfe, 0xf5, 0x7b, 0x0d, 0x2a, 0xa5
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
    
    Web3 web3(1); // Mainnet
    Crypto crypto(&web3);
    crypto.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    const char* message = "Test message";
    uint8_t messageHash[32];
    
    // Create personal message hash with EIP-191 prefix
    // Format: "\x19Ethereum Signed Message:\n" + length_as_string + message
    // For "Test message" (12 bytes): "\x19Ethereum Signed Message:\n12Test message"
    uint8_t fullMessage[12 + 28]; // 12 for message, 28 for prefix ("\x19Ethereum Signed Message:\n12")
    uint8_t* p = fullMessage;
    *p++ = 0x19;
    memcpy(p, "Ethereum Signed Message:\n12", 27);
    p += 27;
    memcpy(p, message, 12);
    
    Crypto::Keccak256(fullMessage, sizeof(fullMessage), messageHash);
    
    uint8_t signature[65];
    bool success = crypto.Sign(messageHash, signature);
    
    if (!success) {
        TEST_FAIL_MESSAGE("Crypto::Sign returned false");
    }
    TEST_ASSERT_TRUE(success);
    
    // Verify non-zero signature
    bool hasNonZero = false;
    for (int i = 0; i < 65; i++) {
        if (signature[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    
    if (!hasNonZero) {
        char msg[150];
        snprintf(msg, 150, "Signature is all zeros. First 8 bytes: %02X %02X %02X %02X %02X %02X %02X %02X",
                 signature[0], signature[1], signature[2], signature[3],
                 signature[4], signature[5], signature[6], signature[7]);
        TEST_FAIL_MESSAGE(msg);
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_signature_recovery() {
    // Test that we can recover the correct address from a signature
    // This verifies the full sign->recover cycle
    
    Web3 web3(1);
    Crypto crypto(&web3);
    crypto.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    const char* message = "Hello Ethereum";
    uint8_t messageHash[32];
    
    // Create personal message hash with EIP-191 prefix
    // Format: "\x19Ethereum Signed Message:\n14Hello Ethereum"
    uint8_t fullMessage[14 + 28]; // 14 for message, 28 for prefix
    uint8_t* p = fullMessage;
    *p++ = 0x19;
    memcpy(p, "Ethereum Signed Message:\n14", 27);
    p += 27;
    memcpy(p, message, 14);
    
    Crypto::Keccak256(fullMessage, sizeof(fullMessage), messageHash);
    
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
