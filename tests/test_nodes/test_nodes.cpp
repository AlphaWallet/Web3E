/**
 * Web3E Node URL Tests
 * 
 * Tests the node URL retrieval functions from nodes.h
 */

#include <Arduino.h>
#include <unity.h>

// Include nodes.h to test the functions
#include "../../src/nodes.h"
#include "../../src/chainIds.h"

void test_get_node_backward_compatibility() {
    // Test that getNode returns a valid URL for known chains
    const char* sepoliaUrl = getNode(SEPOLIA_ID);
    TEST_ASSERT_NOT_NULL(sepoliaUrl);
    TEST_ASSERT_TRUE(strlen(sepoliaUrl) > 0);
    
    const char* mainnetUrl = getNode(MAINNET_ID);
    TEST_ASSERT_NOT_NULL(mainnetUrl);
    TEST_ASSERT_TRUE(strlen(mainnetUrl) > 0);
}

void test_get_node_unknown_chain() {
    // Test that getNode returns empty string for unknown chain
    const char* unknownUrl = getNode(999999);
    TEST_ASSERT_NOT_NULL(unknownUrl);
    TEST_ASSERT_EQUAL_STRING("", unknownUrl);
}

void test_get_node_urls_primary() {
    // Test that getNodeUrls(chainId, 0) returns the primary URL
    const char* sepoliaUrl = getNode(SEPOLIA_ID);
    const char* sepoliaUrlAlt = getNodeUrls(SEPOLIA_ID, 0);
    
    TEST_ASSERT_NOT_NULL(sepoliaUrlAlt);
    TEST_ASSERT_EQUAL_STRING(sepoliaUrl, sepoliaUrlAlt);
}

void test_get_node_urls_alternates() {
    // Test that chains with multiple URLs can return alternates
    // Sepolia should have up to 3 URLs after update
    const char* url0 = getNodeUrls(SEPOLIA_ID, 0);
    const char* url1 = getNodeUrls(SEPOLIA_ID, 1);
    
    TEST_ASSERT_NOT_NULL(url0);
    TEST_ASSERT_TRUE(strlen(url0) > 0);
    
    // At least one alternate should exist for updated chains
    // (url1 could be NULL if only 1 URL available)
    if (url1 != NULL) {
        TEST_ASSERT_TRUE(strlen(url1) > 0);
        // Primary and alternate should be different
        TEST_ASSERT_TRUE(strcmp(url0, url1) != 0);
    }
}

void test_get_node_urls_out_of_range() {
    // Test that invalid indices return NULL
    const char* url = getNodeUrls(SEPOLIA_ID, 10);
    TEST_ASSERT_NULL(url);
    
    // Negative index should also return NULL
    url = getNodeUrls(SEPOLIA_ID, -1);
    TEST_ASSERT_NULL(url);
}

void test_get_node_urls_unknown_chain() {
    // Test that unknown chain returns NULL
    const char* url = getNodeUrls(999999, 0);
    TEST_ASSERT_NULL(url);
}

void test_multiple_chains_have_urls() {
    // Test that several known chains have valid URLs
    long long testChains[] = {
        MAINNET_ID,
        SEPOLIA_ID,
        POLYGON_ID,
        BASE_MAINNET_ID,
        LINEA_MAINNET_ID,
        LINEA_SEPOLIA_ID,
        SEI_MAINNET_ID,
        SEI_TESTNET_ID,
        MONAD_MAINNET_ID,
        ARBITRUM_MAIN_ID,
        OPTIMISTIC_MAIN_ID
    };
    
    int numChains = sizeof(testChains) / sizeof(testChains[0]);
    for (int i = 0; i < numChains; i++) {
        const char* url = getNode(testChains[i]);
        TEST_ASSERT_NOT_NULL(url);
        TEST_ASSERT_TRUE(strlen(url) > 0);
    }
}

void test_chain_with_single_url() {
    // Test Sei Testnet which currently has only 1 URL in the feed
    const char* url0 = getNodeUrls(SEI_TESTNET_ID, 0);
    const char* url1 = getNodeUrls(SEI_TESTNET_ID, 1);
    const char* url2 = getNodeUrls(SEI_TESTNET_ID, 2);
    
    // Primary URL should exist
    TEST_ASSERT_NOT_NULL(url0);
    TEST_ASSERT_TRUE(strlen(url0) > 0);
    
    // Alternates should return NULL (only 1 URL available)
    TEST_ASSERT_NULL(url1);
    TEST_ASSERT_NULL(url2);
}

void test_node_urls_consistency() {
    // Test that getNodeUrls is consistent across calls
    const char* url1 = getNodeUrls(MAINNET_ID, 0);
    const char* url2 = getNodeUrls(MAINNET_ID, 0);
    
    TEST_ASSERT_NOT_NULL(url1);
    TEST_ASSERT_NOT_NULL(url2);
    TEST_ASSERT_EQUAL_STRING(url1, url2);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_get_node_backward_compatibility);
    RUN_TEST(test_get_node_unknown_chain);
    RUN_TEST(test_get_node_urls_primary);
    RUN_TEST(test_get_node_urls_alternates);
    RUN_TEST(test_get_node_urls_out_of_range);
    RUN_TEST(test_get_node_urls_unknown_chain);
    RUN_TEST(test_multiple_chains_have_urls);
    RUN_TEST(test_chain_with_single_url);
    RUN_TEST(test_node_urls_consistency);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
