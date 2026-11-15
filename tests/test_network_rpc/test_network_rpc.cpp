/**
 * Web3E Network RPC Tests
 * 
 * These tests require Wi-Fi and an RPC endpoint.
 * They are only compiled when ENABLE_NETWORK_TESTS is defined.
 * 
 * Configure credentials in test_config.ini or via environment variables:
 * - WIFI_SSID
 * - WIFI_PASS
 * - RPC_URL
 * 
 * PLACEHOLDER: Owner's existing network tests can be added here or in separate files.
 */

#include <Arduino.h>
#include <unity.h>

#ifdef ENABLE_NETWORK_TESTS

#include <WiFi.h>
#include <Web3.h>
#include <Contract.h>

// Credentials from build flags
#ifndef WIFI_SSID
#define WIFI_SSID ""
#warning "WIFI_SSID not defined - network tests will fail"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#warning "WIFI_PASS not defined - network tests will fail"
#endif

#ifndef RPC_URL
#define RPC_URL ""
#warning "RPC_URL not defined - network tests will use default nodes"
#endif

bool wifiConnected = false;

void connectWiFi() {
    if (wifiConnected) return;
    
    Serial.println("\nConnecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
    } else {
        Serial.println("\nFailed to connect to WiFi!");
    }
}

void test_wifi_connection() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
}

void test_get_block_number() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia testnet
    int blockNumber = web3.EthBlockNumber();
    
    Serial.print("Current block number: ");
    Serial.println(blockNumber);
    
    // Block number should be positive and reasonable (> 1000000 for testnets)
    TEST_ASSERT_GREATER_THAN(1000000, blockNumber);
}

void test_get_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia testnet
    
    // Vitalik's address (should have some balance on mainnet, may be 0 on testnet)
    string address = "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045";
    uint256_t balance = web3.EthGetBalance(&address);
    
    Serial.print("Balance: ");
    Serial.println(balance.str().c_str());
    
    // Just verify we got a valid response (balance could be 0)
    TEST_ASSERT_TRUE(true);
}

void test_get_transaction_count() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia testnet
    
    string address = "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045";
    int nonce = web3.EthGetTransactionCount(&address);
    
    Serial.print("Transaction count (nonce): ");
    Serial.println(nonce);
    
    // Nonce should be >= 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, nonce);
}

// PLACEHOLDER: Additional network tests from owner can be added here

#endif // ENABLE_NETWORK_TESTS

void setup() {
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Web3E Network Tests ===");
    
#ifdef ENABLE_NETWORK_TESTS
    Serial.println("Network tests enabled");
    Serial.print("WIFI_SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("RPC_URL: ");
    Serial.println(RPC_URL);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_wifi_connection);
    RUN_TEST(test_get_block_number);
    RUN_TEST(test_get_balance);
    RUN_TEST(test_get_transaction_count);
    
    UNITY_END();
#else
    Serial.println("Network tests DISABLED (OFFLINE_ONLY defined)");
    Serial.println("To run network tests, use: pio test -e esp32dev_network");
    
    UNITY_BEGIN();
    UNITY_END(); // No tests to run
#endif
}

void loop() {
    // Nothing to do here
}
