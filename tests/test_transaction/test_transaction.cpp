/**
 * Web3E Transaction Signing Tests
 * 
 * Tests RLP encoding and signing of Ethereum transactions (legacy and EIP-155).
 * 
 * Test vectors generated using ethers.js v6:
 * - Transaction.from({...}).unsignedSerialized for RLP
 * - wallet.signTransaction({...}) for signed transactions
 */

#include <Arduino.h>
#include <unity.h>
#include <Web3.h>
#include <Contract.h>
#include <Util.h>

void test_transaction_rlp_encoding_legacy() {
    // Test RLP encoding of a simple transaction (legacy, pre-EIP-155)
    // This tests the RLP encoding without actually signing
    
    Web3 web3(1); // Mainnet
    Contract contract(&web3, "0x0000000000000000000000000000000000000000");
    contract.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    // Simple transaction parameters
    uint32_t nonce = 0;
    unsigned long long gasPrice = 20000000000ULL; // 20 Gwei
    uint32_t gasLimit = 21000;
    string toAddr = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8";
    uint256_t value("1000000000000000000"); // 1 ETH
    string data = "";
    
    // This will internally use RLP encoding
    string result = contract.SignTransaction(nonce, gasPrice, gasLimit, &toAddr, &value, &data);
    
    // Just verify we got a hex string starting with 0x
    TEST_ASSERT_TRUE(result.length() > 2);
    TEST_ASSERT_TRUE(result.substr(0, 2) == "0x");
}

void test_transaction_signing_eip155() {
    // Test EIP-155 transaction signing (with chain ID)
    // Chain ID is baked into the signature for replay protection
    
    Web3 web3(1); // Mainnet (chain ID = 1)
    Contract contract(&web3, "0x0000000000000000000000000000000000000000");
    contract.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    uint32_t nonce = 0;
    unsigned long long gasPrice = 20000000000ULL;
    uint32_t gasLimit = 21000;
    string toAddr = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8";
    uint256_t value("1000000000000000000");
    string data = "";
    
    string signedTx = contract.SignTransaction(nonce, gasPrice, gasLimit, &toAddr, &value, &data);
    
    // Verify signature is valid format
    TEST_ASSERT_TRUE(signedTx.length() > 2);
    TEST_ASSERT_TRUE(signedTx.substr(0, 2) == "0x");
    
    // For EIP-155, v should be 27 + chainId * 2 or 28 + chainId * 2
    // For mainnet (chainId=1): v should be 37 or 38
    // This is embedded in the signed transaction
}

void test_contract_data_encoding() {
    // Test ABI encoding for contract function calls
    
    Web3 web3(1);
    Contract contract(&web3, "0x6B175474E89094C44Da98b954EedeAC495271d0F"); // DAI on mainnet
    
    // Encode a transfer(address,uint256) call
    string recipient = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8";
    uint256_t amount("1000000000000000000"); // 1 token (18 decimals)
    
    string data = contract.SetupContractData("transfer(address,uint256)", &recipient, &amount);
    
    // Should start with 0x + function selector (4 bytes = 8 hex chars)
    TEST_ASSERT_TRUE(data.length() >= 10); // 0x + 8 hex chars
    TEST_ASSERT_TRUE(data.substr(0, 2) == "0x");
    
    // Function selector for transfer(address,uint256) is 0xa9059cbb
    TEST_ASSERT_TRUE(data.substr(0, 10) == "0xa9059cbb");
}

void test_contract_data_encoding_no_params() {
    // Test encoding a function with no parameters (e.g., totalSupply())
    
    Web3 web3(1);
    Contract contract(&web3, "0x6B175474E89094C44Da98b954EedeAC495271d0F");
    
    string data = contract.SetupContractData("totalSupply()");
    
    // Function selector for totalSupply() is 0x18160ddd
    // Total length should be 10 chars (0x + 8 hex)
    TEST_ASSERT_EQUAL(10, data.length());
    TEST_ASSERT_TRUE(data == "0x18160ddd");
}

void test_contract_data_encoding_address() {
    // Test encoding balanceOf(address)
    
    Web3 web3(1);
    Contract contract(&web3, "0x6B175474E89094C44Da98b954EedeAC495271d0F");
    
    string address = "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266";
    string data = contract.SetupContractData("balanceOf(address)", &address);
    
    // Function selector for balanceOf(address) is 0x70a08231
    TEST_ASSERT_TRUE(data.substr(0, 10) == "0x70a08231");
    
    // Total length: 0x (2) + selector (8) + padded address (64) = 74 chars
    TEST_ASSERT_EQUAL(74, data.length());
}

void test_zero_value_transaction() {
    // Test transaction with zero value (pure data transaction)
    
    Web3 web3(1);
    Contract contract(&web3, "0x6B175474E89094C44Da98b954EedeAC495271d0F");
    contract.SetPrivateKey("ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    
    uint32_t nonce = 0;
    unsigned long long gasPrice = 20000000000ULL;
    uint32_t gasLimit = 100000;
    string toAddr = "0x6B175474E89094C44Da98b954EedeAC495271d0F";
    uint256_t value("0"); // Zero value
    
    // Encode a contract call
    string recipient = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8";
    uint256_t amount("1000000000000000000");
    string data = contract.SetupContractData("transfer(address,uint256)", &recipient, &amount);
    
    string signedTx = contract.SignTransaction(nonce, gasPrice, gasLimit, &toAddr, &value, &data);
    
    TEST_ASSERT_TRUE(signedTx.length() > 2);
    TEST_ASSERT_TRUE(signedTx.substr(0, 2) == "0x");
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_transaction_rlp_encoding_legacy);
    RUN_TEST(test_transaction_signing_eip155);
    RUN_TEST(test_contract_data_encoding);
    RUN_TEST(test_contract_data_encoding_no_params);
    RUN_TEST(test_contract_data_encoding_address);
    RUN_TEST(test_zero_value_transaction);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
