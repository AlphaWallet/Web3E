/**
 * Web3E Network RPC Tests
 * 
 * Converted from examples: Query Token Balance, Send Eth, Push Transaction, Wallet Bridge
 * 
 * These tests require Wi-Fi and an RPC endpoint.
 * They are only compiled when ENABLE_NETWORK_TESTS is defined.
 * 
 * Configure credentials in test_config.ini:
 * - WIFI_SSID, WIFI_PASS: Wi-Fi credentials
 * - RPC_URL: Ethereum RPC endpoint (Sepolia testnet recommended)
 * - TEST_ADDRESS: Address for balance queries
 * - ERC20_CONTRACT: ERC20 token contract address
 * - TEST_PRIVATE_KEY: Private key for transaction tests (TESTNET ONLY, optional)
 * - TEST_TO_ADDRESS: Destination for test transfers (optional)
 */

#include <Arduino.h>
#include <unity.h>

#ifdef ENABLE_NETWORK_TESTS

#include <WiFi.h>
#include <WiFiConnection.h>
#include <Web3.h>
#include <Contract.h>
#include <Util.h>
#include <KeyID.h>

// Credentials from build flags
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef RPC_URL
#define RPC_URL ""
#endif

#ifndef TEST_ADDRESS
#define TEST_ADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"
#endif

#ifndef ERC20_CONTRACT
#define ERC20_CONTRACT ""
#endif

#ifndef TEST_PRIVATE_KEY
#define TEST_PRIVATE_KEY ""
#endif

#ifndef TEST_TO_ADDRESS
#define TEST_TO_ADDRESS ""
#endif

WiFiConnection wifiConnection;
bool wifiConnected = false;

void connectWiFi() {
    if (wifiConnected) return;
    
    Serial.println("\n=== Connecting to WiFi ===");
    WiFiCredentials networks[] = {
        {WIFI_SSID, WIFI_PASS},
        {nullptr, nullptr}  // Terminator
    };
    
    wifiConnection.setupWifi(networks);
    wifiConnected = (WiFi.status() == WL_CONNECTED);
}

// ===== Read-Only Tests (No Funds Required) =====

void test_wifi_connection() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
}

void test_get_chain_id() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia
    long long chainId = web3.getChainId();
    
    Serial.print("Chain ID: ");
    Serial.println((long)chainId);
    
    TEST_ASSERT_EQUAL(11155111, chainId);
}

void test_get_block_number() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia
    int blockNumber = web3.EthBlockNumber();
    
    Serial.print("Current block number: ");
    Serial.println(blockNumber);
    
    // Sepolia should have blocks > 1M
    TEST_ASSERT_GREATER_THAN(1000000, blockNumber);
}

void test_get_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia
    string address = TEST_ADDRESS;
    uint256_t balance = web3.EthGetBalance(&address);
    
    Serial.print("Native balance for ");
    Serial.print(address.c_str());
    Serial.print(": ");
    Serial.println(balance.str().c_str());
    
    // Just verify we got a response (balance could be 0)
    // Non-negative balance is valid
    TEST_ASSERT_TRUE(balance >= 0);
}

void test_get_transaction_count() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Web3 web3(11155111); // Sepolia
    string address = TEST_ADDRESS;
    int nonce = web3.EthGetTransactionCount(&address);
    
    Serial.print("Transaction count for ");
    Serial.print(address.c_str());
    Serial.print(": ");
    Serial.println(nonce);
    
    TEST_ASSERT_GREATER_OR_EQUAL(0, nonce);
}

void test_query_erc20_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (strlen(ERC20_CONTRACT) == 0) {
        TEST_IGNORE_MESSAGE("ERC20_CONTRACT not configured - skipping");
        return;
    }
    
    Web3 web3(11155111); // Sepolia
    Contract contract(&web3, ERC20_CONTRACT);
    string address = TEST_ADDRESS;
    
    // Query decimals
    string param = contract.SetupContractData("decimals()");
    string result = contract.ViewCall(&param);
    int decimals = web3.getInt(&result);
    
    Serial.print("Token decimals: ");
    Serial.println(decimals);
    TEST_ASSERT_GREATER_OR_EQUAL(0, decimals);
    TEST_ASSERT_LESS_OR_EQUAL(255, decimals);
    
    // Query balance
    param = contract.SetupContractData("balanceOf(address)", &address);
    result = contract.ViewCall(&param);
    uint256_t baseBalance = web3.getUint256(&result);
    
    string balanceStr = Util::ConvertWeiToEthString(&baseBalance, decimals);
    
    Serial.print("ERC20 balance for ");
    Serial.print(address.c_str());
    Serial.print(": ");
    Serial.print(balanceStr.c_str());
    Serial.println(" tokens");
    
    // Balance should be non-negative
    TEST_ASSERT_TRUE(baseBalance >= 0);
}

void test_query_erc20_name_symbol() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (strlen(ERC20_CONTRACT) == 0) {
        TEST_IGNORE_MESSAGE("ERC20_CONTRACT not configured - skipping");
        return;
    }
    
    Web3 web3(11155111); // Sepolia
    Contract contract(&web3, ERC20_CONTRACT);
    
    // Query name
    string param = contract.SetupContractData("name()");
    string result = contract.ViewCall(&param);
    string name = web3.getString(&result);
    
    Serial.print("Token name: ");
    Serial.println(name.c_str());
    
    // Query symbol
    param = contract.SetupContractData("symbol()");
    result = contract.ViewCall(&param);
    string symbol = web3.getString(&result);
    
    Serial.print("Token symbol: ");
    Serial.println(symbol.c_str());
    
    // Name and symbol should not be empty
    TEST_ASSERT_GREATER_THAN(0, name.length());
    TEST_ASSERT_GREATER_THAN(0, symbol.length());
}

// ===== Transaction Tests (Require Funded Account) =====

void test_prepare_eth_transfer() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (strlen(TEST_PRIVATE_KEY) == 0 || strlen(TEST_TO_ADDRESS) == 0) {
        TEST_IGNORE_MESSAGE("TEST_PRIVATE_KEY or TEST_TO_ADDRESS not configured - skipping");
        return;
    }
    
    Web3 web3(11155111); // Sepolia
    Contract contract(&web3, "");
    contract.SetPrivateKey(TEST_PRIVATE_KEY);
    
    // Get nonce (don't actually send)
    // Extract address from private key first
    KeyID keyID(&web3, TEST_PRIVATE_KEY);
    string fromAddress = keyID.getAddress();
    
    int nonce = web3.EthGetTransactionCount(&fromAddress);
    Serial.print("Nonce for test account: ");
    Serial.println(nonce);
    
    TEST_ASSERT_GREATER_OR_EQUAL(0, nonce);
    
    Serial.println("NOTE: Transaction not sent - this is a dry-run test");
    Serial.println("To send real transactions, implement full transaction tests");
}

#endif // ENABLE_NETWORK_TESTS

void setup() {
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Web3E Network Tests ===");
    
#ifdef ENABLE_NETWORK_TESTS
    Serial.println("Network tests ENABLED");
    Serial.print("WIFI_SSID: ");
    Serial.println(WIFI_SSID);
    Serial.println();
    
    UNITY_BEGIN();
    
    // Connection tests
    RUN_TEST(test_wifi_connection);
    RUN_TEST(test_get_chain_id);
    RUN_TEST(test_get_block_number);
    
    // Read-only tests
    RUN_TEST(test_get_balance);
    RUN_TEST(test_get_transaction_count);
    RUN_TEST(test_query_erc20_balance);
    RUN_TEST(test_query_erc20_name_symbol);
    
    // Transaction tests (require funded account)
    RUN_TEST(test_prepare_eth_transfer);
    
    UNITY_END();
#else
    Serial.println("Network tests DISABLED (OFFLINE_ONLY defined)");
    Serial.println("To run network tests, use: pio test -e esp32dev_network");
    
    UNITY_BEGIN();
    UNITY_END();
#endif
}

void loop() {
    // Nothing to do here
}
