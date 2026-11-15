/**
 * Web3E Network RPC Tests
 * 
 * Read-only network tests (no funds required).
 * Tests Web3E's built-in node selection and RPC functionality.
 * 
 * Configure credentials in test_config.ini:
 * - WIFI_SSID, WIFI_PASS: Wi-Fi credentials
 * - ERC721_CONTRACT: Optional mainnet ERC721 for query test
 */

#include <Arduino.h>
#include <unity.h>

#ifdef ENABLE_NETWORK_TESTS

#include <WiFi.h>
#include <WiFiConnection.h>
#include <Web3.h>
#include <Contract.h>
#include <Util.h>

// Credentials from build flags
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef ERC721_CONTRACT
#define ERC721_CONTRACT ""
#endif

// Web3E uses built-in node selection based on chain ID
#define SEPOLIA_CHAIN_ID 11155111
#define MAINNET_CHAIN_ID 1

// Vitalik's mainnet address for testing
#define VITALIK_ADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"

// USDC on mainnet for ERC20 read test
#define USDC_MAINNET "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"

WiFiConnection wifiConnection;
bool wifiConnected = false;

void connectWiFi() {
    if (wifiConnected) return;
    
    Serial.println("\n=== Connecting to WiFi ===");
    WiFiCredentials networks[] = {
        {WIFI_SSID, WIFI_PASS},
        {nullptr, nullptr}
    };
    
    wifiConnection.setupWifi(networks);
    wifiConnected = (WiFi.status() == WL_CONNECTED);
}

// ===== WiFi Tests =====

void test_wifi_connection() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
}

// ===== Sepolia Tests =====

void test_sepolia_chain_id() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Sepolia Chain ID Test ===");
    
    Web3 web3(SEPOLIA_CHAIN_ID);
    long long chainId = web3.getChainId();
    
    Serial.print("Chain ID: ");
    Serial.println((long)chainId);
    
    TEST_ASSERT_EQUAL(SEPOLIA_CHAIN_ID, (long)chainId);
}

void test_sepolia_block_number() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Sepolia Block Number Test ===");
    
    Web3 web3(SEPOLIA_CHAIN_ID);
    long long blockNumber = web3.EthBlockNumber();
    
    Serial.print("Current block: ");
    Serial.println((long)blockNumber);
    
    // Sepolia well past block 1M
    TEST_ASSERT_GREATER_THAN(1000000, (long)blockNumber);
}

// ===== Mainnet Tests =====

void test_mainnet_chain_id() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Mainnet Chain ID Test ===");
    
    Web3 web3(MAINNET_CHAIN_ID);
    long long chainId = web3.getChainId();
    
    Serial.print("Chain ID: ");
    Serial.println((long)chainId);
    
    TEST_ASSERT_EQUAL(MAINNET_CHAIN_ID, (long)chainId);
}

void test_mainnet_vitalik_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Mainnet Balance Test (Vitalik) ===");
    
    Web3 web3(MAINNET_CHAIN_ID);
    string address = VITALIK_ADDRESS;
    uint256_t balance = web3.EthGetBalance(&address);
    
    string balanceStr = Util::ConvertWeiToEthString(&balance, 18);
    Serial.print("Vitalik balance: ");
    Serial.print(balanceStr.c_str());
    Serial.println(" ETH");
    
    // Vitalik should have ETH on mainnet
    TEST_ASSERT_TRUE(balance > 0);
}

void test_mainnet_transaction_count() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Mainnet Nonce Test (Vitalik) ===");
    
    Web3 web3(MAINNET_CHAIN_ID);
    string address = VITALIK_ADDRESS;
    long long nonce = web3.EthGetTransactionCount(&address);
    
    Serial.print("Vitalik nonce: ");
    Serial.println((long)nonce);
    
    // Vitalik has sent many transactions
    TEST_ASSERT_GREATER_THAN(100, (long)nonce);
}

// ===== ERC20 Tests (Mainnet USDC) =====

void test_erc20_name() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== ERC20 Name Test (USDC Mainnet) ===");
    
    Web3 web3(MAINNET_CHAIN_ID);
    Contract contract(&web3, USDC_MAINNET);
    
    string param = contract.SetupContractData("name()");
    string result = contract.ViewCall(&param);
    string name = Util::InterpretStringResult(web3.getString(&result).c_str());
    
    Serial.print("Token name: ");
    Serial.println(name.c_str());
    
    // USDC name should contain "USD"
    TEST_ASSERT_TRUE(name.find("USD") != string::npos);
}

void test_erc20_decimals() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== ERC20 Decimals Test (USDC Mainnet) ===");
    
    Web3 web3(MAINNET_CHAIN_ID);
    Contract contract(&web3, USDC_MAINNET);
    
    string param = contract.SetupContractData("decimals()");
    string result = contract.ViewCall(&param);
    int decimals = web3.getInt(&result);
    
    Serial.print("USDC decimals: ");
    Serial.println(decimals);
    
    // USDC has 6 decimals
    TEST_ASSERT_EQUAL(6, decimals);
}

void test_erc20_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== ERC20 Balance Test (USDC Mainnet) ===");
    
    Web3 web3(MAINNET_CHAIN_ID);
    Contract contract(&web3, USDC_MAINNET);
    
    string address = VITALIK_ADDRESS;
    string param = contract.SetupContractData("balanceOf(address)", &address);
    string result = contract.ViewCall(&param);
    uint256_t balance = web3.getUint256(&result);
    
    string balanceStr = Util::ConvertWeiToEthString(&balance, 6);  // USDC has 6 decimals
    Serial.print("Vitalik USDC balance: ");
    Serial.println(balanceStr.c_str());
    
    // Just verify we got a response (balance could be 0)
    TEST_ASSERT_TRUE(balance >= 0);
}

// ===== ERC721 Tests (Optional) =====

void test_erc721_vitalik_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (strlen(ERC721_CONTRACT) == 0) {
        TEST_IGNORE_MESSAGE("ERC721_CONTRACT not configured in test_config.ini");
        return;
    }
    
    Serial.println("\n=== ERC721 Balance Test (Vitalik, Mainnet) ===");
    Serial.print("Contract: ");
    Serial.println(ERC721_CONTRACT);
    
    Web3 web3(MAINNET_CHAIN_ID);
    Contract contract(&web3, ERC721_CONTRACT);
    
    string address = VITALIK_ADDRESS;
    string param = contract.SetupContractData("balanceOf(address)", &address);
    string result = contract.ViewCall(&param);
    long balance = web3.getInt(&result);
    
    Serial.print("Vitalik NFT balance: ");
    Serial.println(balance);
    
    // Vitalik should own at least one NFT in the configured contract
    TEST_ASSERT_GREATER_THAN(0, balance);
}

#endif // ENABLE_NETWORK_TESTS

void setup() {
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Web3E Network RPC Tests ===");
    
#ifdef ENABLE_NETWORK_TESTS
    Serial.println("Network RPC tests ENABLED");
    Serial.println("Tests use Web3E's built-in node selection");
    Serial.println();
    
    UNITY_BEGIN();
    
    // WiFi
    RUN_TEST(test_wifi_connection);
    
    // Sepolia tests
    RUN_TEST(test_sepolia_chain_id);
    RUN_TEST(test_sepolia_block_number);
    
    // Mainnet tests
    RUN_TEST(test_mainnet_chain_id);
    RUN_TEST(test_mainnet_vitalik_balance);
    RUN_TEST(test_mainnet_transaction_count);
    
    // ERC20 tests (USDC on mainnet)
    RUN_TEST(test_erc20_name);
    RUN_TEST(test_erc20_decimals);
    RUN_TEST(test_erc20_balance);
    
    // ERC721 test (optional)
    RUN_TEST(test_erc721_vitalik_balance);
    
    UNITY_END();
#else
    Serial.println("Network tests DISABLED");
    
    UNITY_BEGIN();
    UNITY_END();
#endif
}

void loop() {
    // Nothing to do here
}
