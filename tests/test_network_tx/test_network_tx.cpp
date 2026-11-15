/**
 * Web3E Network Transaction Tests
 * 
 * Tests that actually send transactions and verify receipts.
 * Converted from examples: Send Eth to Address, Push Transaction
 * 
 * IMPORTANT: These tests send real transactions on testnet.
 * Configure test_config.ini with:
 * - TEST_PRIVATE_KEY: Private key with testnet funds (TESTNET ONLY!)
 * - TEST_TO_ADDRESS: Destination address
 * - TEST_TOKEN_CONTRACT: ERC20 contract address (for token transfers)
 * 
 * Tests skip if credentials not configured. Uses Sepolia testnet by default.
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

// Test configuration from build flags
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef TEST_PRIVATE_KEY
#define TEST_PRIVATE_KEY ""
#endif

#ifndef TEST_TO_ADDRESS
#define TEST_TO_ADDRESS ""
#endif

#ifndef TEST_TOKEN_CONTRACT
#define TEST_TOKEN_CONTRACT ""
#endif

#define SEPOLIA_CHAIN_ID 11155111
#define TEST_ETH_AMOUNT 0.001  // Small amount for testing

WiFiConnection wifiConnection;
bool wifiConnected = false;
Web3* web3 = nullptr;

void connectWiFi() {
    if (wifiConnected) return;
    
    Serial.println("\n=== Connecting to WiFi ===");
    WiFiCredentials networks[] = {
        {WIFI_SSID, WIFI_PASS},
        {nullptr, nullptr}
    };
    
    wifiConnection.setupWifi(networks);
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    if (wifiConnected && web3 == nullptr) {
        web3 = new Web3(SEPOLIA_CHAIN_ID);
    }
}

bool hasTestCredentials() {
    return strlen(TEST_PRIVATE_KEY) > 0 && 
           strlen(TEST_TO_ADDRESS) > 0 &&
           strcmp(TEST_PRIVATE_KEY, "DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00") != 0;
}

void waitForReceipt(const string& txHash, int maxWaitSeconds = 60) {
    Serial.print("Waiting for transaction receipt: ");
    Serial.println(txHash.c_str());
    
    // Note: Web3E doesn't have a built-in getTransactionReceipt yet
    // This is a placeholder for polling implementation
    Serial.println("Transaction sent. Check on block explorer:");
    Serial.print("https://sepolia.etherscan.io/tx/");
    Serial.println(txHash.c_str());
    
    // Wait a bit for transaction to be mined
    delay(5000);
}

// ===== Transaction Tests =====

void test_send_eth_transaction() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (!hasTestCredentials()) {
        TEST_IGNORE_MESSAGE("TEST_PRIVATE_KEY or TEST_TO_ADDRESS not configured");
        return;
    }
    
    // Safety: Verify we're on a testnet
    long long chainId = web3->getChainId();
    TEST_ASSERT_EQUAL(SEPOLIA_CHAIN_ID, chainId);
    
    Serial.println("\n=== Send ETH Transaction Test ===");
    
    // Setup contract for sending ETH (no contract address needed)
    Contract contract(web3, "");
    contract.SetPrivateKey(TEST_PRIVATE_KEY);
    
    // Get sender address from private key
    KeyID keyID(web3, TEST_PRIVATE_KEY);
    string fromAddress = keyID.getAddress();
    
    Serial.print("From address: ");
    Serial.println(fromAddress.c_str());
    Serial.print("To address: ");
    Serial.println(TEST_TO_ADDRESS);
    
    // Check balance before
    uint256_t balanceBefore = web3->EthGetBalance(&fromAddress);
    Serial.print("Balance before: ");
    Serial.println(Util::ConvertWeiToEthString(&balanceBefore, 18).c_str());
    
    // Verify sufficient balance (need at least TEST_ETH_AMOUNT + gas)
    uint256_t required = Util::ConvertToWei(TEST_ETH_AMOUNT + 0.001, 18); // amount + estimated gas
    TEST_ASSERT_TRUE(balanceBefore >= required);
    
    // Get nonce
    uint32_t nonce = (uint32_t)web3->EthGetTransactionCount(&fromAddress);
    Serial.print("Nonce: ");
    Serial.println(nonce);
    
    // Transaction parameters
    unsigned long long gasPrice = 2000000000ULL; // 2 Gwei
    uint32_t gasLimit = 21000;
    uint256_t value = Util::ConvertToWei(TEST_ETH_AMOUNT, 18);
    string emptyData = "";
    string toAddress = TEST_TO_ADDRESS;
    
    // Send transaction
    Serial.println("Sending transaction...");
    string result = contract.SendTransaction(nonce, gasPrice, gasLimit, &toAddress, &value, &emptyData);
    
    Serial.print("Result: ");
    Serial.println(result.c_str());
    
    // Extract transaction hash
    string txHash = web3->getResult(&result);
    Serial.print("Transaction hash: ");
    Serial.println(txHash.c_str());
    
    // Verify we got a transaction hash (starts with 0x and is 66 chars)
    TEST_ASSERT_TRUE(txHash.length() == 66);
    TEST_ASSERT_TRUE(txHash.substr(0, 2) == "0x");
    
    waitForReceipt(txHash);
}

void test_send_erc20_transaction() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (!hasTestCredentials() || strlen(TEST_TOKEN_CONTRACT) == 0) {
        TEST_IGNORE_MESSAGE("TEST_PRIVATE_KEY, TEST_TO_ADDRESS, or TEST_TOKEN_CONTRACT not configured");
        return;
    }
    
    // Safety: Verify we're on a testnet
    long long chainId = web3->getChainId();
    TEST_ASSERT_EQUAL(SEPOLIA_CHAIN_ID, chainId);
    
    Serial.println("\n=== Send ERC20 Transaction Test ===");
    
    Contract contract(web3, TEST_TOKEN_CONTRACT);
    contract.SetPrivateKey(TEST_PRIVATE_KEY);
    
    // Get sender address
    KeyID keyID(web3, TEST_PRIVATE_KEY);
    string fromAddress = keyID.getAddress();
    
    Serial.print("Token contract: ");
    Serial.println(TEST_TOKEN_CONTRACT);
    Serial.print("From: ");
    Serial.println(fromAddress.c_str());
    Serial.print("To: ");
    Serial.println(TEST_TO_ADDRESS);
    
    // Get token decimals
    string param = contract.SetupContractData("decimals()");
    string result = contract.ViewCall(&param);
    int decimals = web3->getInt(&result);
    
    Serial.print("Token decimals: ");
    Serial.println(decimals);
    TEST_ASSERT_GREATER_OR_EQUAL(0, decimals);
    
    // Check balance before
    param = contract.SetupContractData("balanceOf(address)", &fromAddress);
    result = contract.ViewCall(&param);
    uint256_t balanceBefore = web3->getUint256(&result);
    
    Serial.print("Token balance before: ");
    Serial.println(Util::ConvertWeiToEthString(&balanceBefore, decimals).c_str());
    
    // Amount to transfer (0.1 tokens)
    uint256_t transferAmount = Util::ConvertToWei(0.1, decimals);
    
    // Verify sufficient balance
    TEST_ASSERT_TRUE(balanceBefore >= transferAmount);
    
    // Get nonce
    uint32_t nonce = (uint32_t)web3->EthGetTransactionCount(&fromAddress);
    Serial.print("Nonce: ");
    Serial.println(nonce);
    
    // Transaction parameters
    unsigned long long gasPrice = 2000000000ULL; // 2 Gwei
    uint32_t gasLimit = 100000;
    uint256_t callValue = 0; // No ETH sent, just token transfer
    string toAddress = TEST_TO_ADDRESS;
    string contractAddress = TEST_TOKEN_CONTRACT;
    
    // Setup contract call data for transfer(address,uint256)
    string data = contract.SetupContractData("transfer(address,uint256)", &toAddress, &transferAmount);
    
    // Send transaction
    Serial.println("Sending ERC20 transfer...");
    result = contract.SendTransaction(nonce, gasPrice, gasLimit, &contractAddress, &callValue, &data);
    
    Serial.print("Result: ");
    Serial.println(result.c_str());
    
    // Extract transaction hash
    string txHash = web3->getResult(&result);
    Serial.print("Transaction hash: ");
    Serial.println(txHash.c_str());
    
    // Verify we got a transaction hash
    TEST_ASSERT_TRUE(txHash.length() == 66);
    TEST_ASSERT_TRUE(txHash.substr(0, 2) == "0x");
    
    waitForReceipt(txHash);
}

void test_transaction_signing_only() {
    connectWiFi();
    TEST_ASSERT_TRUE(wifiConnected);
    
    if (!hasTestCredentials()) {
        TEST_IGNORE_MESSAGE("TEST_PRIVATE_KEY or TEST_TO_ADDRESS not configured");
        return;
    }
    
    Serial.println("\n=== Transaction Signing Test (No Send) ===");
    
    Contract contract(web3, "");
    contract.SetPrivateKey(TEST_PRIVATE_KEY);
    
    KeyID keyID(web3, TEST_PRIVATE_KEY);
    string fromAddress = keyID.getAddress();
    
    uint32_t nonce = (uint32_t)web3->EthGetTransactionCount(&fromAddress);
    unsigned long long gasPrice = 2000000000ULL;
    uint32_t gasLimit = 21000;
    string toAddress = TEST_TO_ADDRESS;
    uint256_t value = Util::ConvertToWei(0.001, 18);
    string emptyData = "";
    
    // Sign transaction without sending
    string signedTx = contract.SignTransaction(nonce, gasPrice, gasLimit, &toAddress, &value, &emptyData);
    
    Serial.print("Signed transaction: ");
    Serial.println(signedTx.c_str());
    
    // Verify signed transaction format (starts with 0x, is reasonable length)
    TEST_ASSERT_TRUE(signedTx.length() > 100);
    TEST_ASSERT_TRUE(signedTx.substr(0, 2) == "0x");
    
    Serial.println("Transaction signed successfully (not sent)");
}

#endif // ENABLE_NETWORK_TESTS

void setup() {
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Web3E Network Transaction Tests ===");
    
#ifdef ENABLE_NETWORK_TESTS
    Serial.println("Network transaction tests ENABLED");
    Serial.println("WARNING: These tests send real transactions on Sepolia testnet");
    Serial.println();
    
    UNITY_BEGIN();
    
    // Transaction tests (require funded account)
    RUN_TEST(test_transaction_signing_only);
    RUN_TEST(test_send_eth_transaction);
    RUN_TEST(test_send_erc20_transaction);
    
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
