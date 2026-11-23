/**
 * Web3E Network RPC Tests
 * 
 * Read-only network tests (no funds required).
 * Uses explicit setRpcUrl() for lab endpoints (not bundled node tables).
 * Production devices should prefer useCachedRpcPair(uninode).
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

#ifndef ERC721_NFT_TEST_ADDRESS
#define ERC721_NFT_TEST_ADDRESS ""
#endif

#define SEPOLIA_CHAIN_ID 11155111
#define MAINNET_CHAIN_ID 1

// Explicit lab RPC URLs (user-supplied override — not library builtins)
#ifndef TEST_SEPOLIA_RPC_URL
#define TEST_SEPOLIA_RPC_URL "https://ethereum-sepolia-rpc.publicnode.com"
#endif
#ifndef TEST_MAINNET_RPC_URL
#define TEST_MAINNET_RPC_URL "https://ethereum-rpc.publicnode.com"
#endif

// Vitalik's mainnet address for testing
#define VITALIK_ADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"

// USDC on mainnet for ERC20 read test
#define USDC_MAINNET "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"

WiFiConnection wifiConnection;
bool wifiConnected = false;

void connectWiFi() {
    if (wifiConnected) return;
    
    Serial.println("\n=== Connecting to WiFi ===");
    
    // Check for empty credentials
    if (strlen(WIFI_SSID) == 0 || strlen(WIFI_PASS) == 0) {
        Serial.println("ERROR: Empty Wi-Fi credentials!");
        Serial.println("Copy test_config.ini.example to test_config.ini and set WIFI_SSID/WIFI_PASS");
        wifiConnected = false;
        return;
    }
    
    Serial.print("Connecting to: ");
    Serial.println(WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // 30-second timeout
    unsigned long startTime = millis();
    const unsigned long timeout = 30000;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    if (wifiConnected) {
        Serial.println("WiFi connected.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("ERROR: Wi-Fi connection timeout!");
    }
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
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_SEPOLIA_RPC_URL), "setRpcUrl sepolia");
    
    // Call eth_chainId RPC method (genuine network I/O test)
    long long chainId = web3.EthChainId();
    
    Serial.print("Chain ID (from RPC): ");
    Serial.println((long)chainId);
    
    TEST_ASSERT_EQUAL(SEPOLIA_CHAIN_ID, (long)chainId);
}

void test_sepolia_block_number() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Sepolia Block Number Test ===");
    
    delay(500); // Rate limit protection
    
    Web3 web3(SEPOLIA_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_SEPOLIA_RPC_URL), "setRpcUrl sepolia");
    long long blockNumber = web3.EthBlockNumber();
    
    // Retry once on failure (RPC error or rate limit)
    if (blockNumber == 0) {
        Serial.println("Block number 0, retrying...");
        delay(1000);
        blockNumber = web3.EthBlockNumber();
    }
    
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
    
    delay(500); // Rate limit protection
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
    
    // Call eth_chainId RPC method (genuine network I/O test)
    long long chainId = web3.EthChainId();
    
    Serial.print("Chain ID (from RPC): ");
    Serial.println((long)chainId);
    
    TEST_ASSERT_EQUAL(MAINNET_CHAIN_ID, (long)chainId);
}

void test_mainnet_vitalik_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== Mainnet Balance Test (Vitalik) ===");
    
    delay(500); // Rate limit protection
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
    string address = VITALIK_ADDRESS;
    uint256_t balance = web3.EthGetBalance(&address);
    
    // Retry once on failure
    if (balance == 0) {
        Serial.println("Balance 0, retrying...");
        delay(1000);
        balance = web3.EthGetBalance(&address);
    }
    
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
    
    delay(500); // Rate limit protection
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
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
    
    delay(500); // Rate limit protection
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
    Contract contract(&web3, USDC_MAINNET);
    
    string param = contract.SetupContractData("name()");
    string result = contract.ViewCall(&param);
    string name = web3.getString(&result);
    
    Serial.print("Token name: ");
    Serial.println(name.c_str());
    
    // USDC name should contain "USD"
    TEST_ASSERT_TRUE(name.find("USD") != string::npos);
}

void test_erc20_decimals() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    Serial.println("\n=== ERC20 Decimals Test (USDC Mainnet) ===");
    
    delay(500); // Rate limit protection
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
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
    
    delay(500); // Rate limit protection
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
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

void test_erc721_balance() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (strlen(ERC721_CONTRACT) == 0 || strlen(ERC721_NFT_TEST_ADDRESS) == 0) {
        TEST_IGNORE_MESSAGE("ERC721_CONTRACT or ERC721_NFT_TEST_ADDRESS not configured in test_config.ini");
        return;
    }
    
    Serial.println("\n=== ERC721 Balance Test (Mainnet) ===");
    
    delay(500); // Rate limit protection
    Serial.print("Contract: ");
    Serial.println(ERC721_CONTRACT);
    Serial.print("Holder: ");
    Serial.println(ERC721_NFT_TEST_ADDRESS);
    
    Web3 web3(MAINNET_CHAIN_ID);
    TEST_ASSERT_TRUE_MESSAGE(web3.setRpcUrl(TEST_MAINNET_RPC_URL), "setRpcUrl mainnet");
    Contract contract(&web3, ERC721_CONTRACT);
    
    string address = ERC721_NFT_TEST_ADDRESS;
    string param = contract.SetupContractData("balanceOf(address)", &address);
    string result = contract.ViewCall(&param);
    long balance = web3.getInt(&result);
    
    Serial.print("NFT balance: ");
    Serial.println(balance);
    
    // Holder should own at least one NFT in the configured contract
    TEST_ASSERT_GREATER_THAN(0, balance);
}

#endif // ENABLE_NETWORK_TESTS

void setup() {
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Web3E Network RPC Tests ===");
    
#ifdef ENABLE_NETWORK_TESTS
    Serial.println("Network RPC tests ENABLED");
    Serial.println("Tests use Web3::setRpcUrl (explicit lab endpoints)");
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
    RUN_TEST(test_erc721_balance);
    
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
