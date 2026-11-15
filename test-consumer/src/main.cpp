/**
 * Minimal test consumer for Web3E library
 * 
 * This project verifies that Web3E can be consumed directly from a git URL
 * in a PlatformIO project without manual configuration.
 */

#include <Arduino.h>
#include <Web3.h>
#include <WiFi.h>

// Test that we can instantiate Web3 objects and use basic functionality
Web3 *web3;

void setup()
{
    Serial.begin(115200);
    Serial.println("\n\nWeb3E Test Consumer");
    Serial.println("===================");
    
    // Test Web3 initialization with a known chain ID
    web3 = new Web3(SEPOLIA_ID);
    Serial.println("✓ Web3 object created successfully");
    
    // Test that we can access basic types
    uint256_t testValue = 1000000000000000000ULL; // 1 ETH in wei
    Serial.print("✓ uint256_t operations work: ");
    Serial.println(testValue.str().c_str());
    
    Serial.println("\nAll basic Web3E functionality tests passed!");
    Serial.println("The library is correctly packaged for PlatformIO consumption.");
}

void loop()
{
    // Nothing to do in loop for this test
    delay(1000);
}
