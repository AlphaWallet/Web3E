#include <Arduino.h>
#include <WiFi.h>
#include <Web3.h>
#include <AploContracts.h>
#include <Util.h>

// ============================================================================
// IMPORTANT SAFETY NOTES - READ BEFORE USE
// ============================================================================
//
// 1. NEVER commit your real private key to version control
// 2. Staking LOCKS your APLO in the contract - ensure you understand the mechanism
// 3. Minimum stake: 1,000 APLO to receive mining rewards (1.0× multiplier)
// 4. Higher stakes increase mining multiplier up to 1.7× (8,000+ APLO)
// 5. Unstaking returns ALL staked APLO and resets multiplier to 0
// 6. Test with small amounts first if possible
// 7. Ensure sufficient balance for stake amount + gas fees
//
// Staking Tier Table (from AploNode builtin/aplo/aplo.go):
//   < 1,000 APLO → multiplier 0   (no mining reward)
//   ≥ 1,000 APLO → multiplier 1.0× (10/10)
//   ≥ 2,000 APLO → multiplier 1.1× (11/10)
//   ≥ 3,000 APLO → multiplier 1.2× (12/10)
//   ...
//   ≥ 8,000 APLO → multiplier 1.7× (17/10, maximum)
//
// ============================================================================

// WiFi credentials - replace with your network details
const char *ssid = "<YOUR_SSID>";
const char *password = "<YOUR_WIFI_PASSWORD>";

// Wallet configuration
// SECURITY: Replace with your actual address and private key
// WARNING: Keep your private key secret! Never share or commit it.
#define MY_ADDRESS "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb"

// CRITICAL: Replace with your actual 64-character hex private key (without 0x prefix)
const char *PRIVATE_KEY = "0000000000000000000000000000000000000000000000000000000000000000";

// Staking parameters
// Amount to stake in APLO (minimum 1,000 APLO for rewards)
#define STAKE_AMOUNT_APLO 1000.0

Web3 *web3;
int wificounter = 0;

void setup_wifi();
void queryStakingStatus(const char *address);
void stakeAplo(double aplo);
void unstakeAplo();
double queryBalance(const char *address);

void setup() 
{
    Serial.begin(115200);
    Serial.println("\n\n=== AploEmbed Staking Example ===\n");
    
    setup_wifi();
    
    // Initialize Web3 with default AploCoin RPC endpoints
    // Uses pub1.aplocoin.com as primary, pub2.aplocoin.com as fallback
    web3 = new Web3();
    
    // Alternative: specify custom RPC endpoint
    // web3 = new Web3("custom-rpc.aplocoin.com");
    
    // Alternative: specify both primary and fallback
    // web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
    
    Serial.println("Web3 initialized with AploCoin RPC endpoints");
    Serial.println("Primary: pub1.aplocoin.com");
    Serial.println("Fallback: pub2.aplocoin.com\n");
    
    Serial.print("Staking Contract: ");
    Serial.println(APLO_STAKING_CONTRACT);
    Serial.println();
    
    // Query current balance
    double balance = queryBalance(MY_ADDRESS);
    Serial.print("My Address: ");
    Serial.println(MY_ADDRESS);
    Serial.print("Current Balance: ");
    Serial.print(balance, 6);
    Serial.println(" APLO\n");
    
    // Query current staking status BEFORE staking
    Serial.println("=== Current Staking Status ===\n");
    queryStakingStatus(MY_ADDRESS);
    
    // Calculate required balance (stake amount + gas buffer)
    double gasBuffer = 0.01;  // Buffer for gas fees
    double requiredBalance = STAKE_AMOUNT_APLO + gasBuffer;
    
    Serial.print("\nAttempting to stake: ");
    Serial.print(STAKE_AMOUNT_APLO, 2);
    Serial.println(" APLO");
    Serial.println();
    
    // Safety check: ensure sufficient balance
    if (balance >= requiredBalance) 
    {
        stakeAplo(STAKE_AMOUNT_APLO);
        
        // Query staking status AFTER staking to show the change
        Serial.println("\n=== Updated Staking Status ===\n");
        queryStakingStatus(MY_ADDRESS);
        
        // Uncomment to test unstaking (returns ALL staked APLO)
        // Serial.println("\n=== Testing Unstake ===\n");
        // unstakeAplo();
        // Serial.println("\n=== Final Staking Status ===\n");
        // queryStakingStatus(MY_ADDRESS);
    }
    else
    {
        Serial.println("ERROR: Insufficient balance!");
        Serial.print("Required: ");
        Serial.print(requiredBalance, 6);
        Serial.print(" APLO (");
        Serial.print(STAKE_AMOUNT_APLO, 6);
        Serial.print(" + ");
        Serial.print(gasBuffer, 6);
        Serial.println(" gas buffer)");
        Serial.print("Available: ");
        Serial.print(balance, 6);
        Serial.println(" APLO");
        Serial.println("\nStaking aborted for safety.");
    }
}

void loop() 
{
    // Staking operations are done once in setup()
    // Add periodic status checks here if needed
    delay(10000);
}

/**
 * Query account balance in APLO
 * Returns balance as double for easy comparison
 */
double queryBalance(const char *address)
{
    string addr = address;
    
    // Get balance in Gaplo (wei)
    uint256_t balanceGaplo = web3->AploGetBalance(&addr);
    
    // Convert to APLO string (18 decimals)
    string balanceStr = Util::ConvertWeiToEthString(&balanceGaplo, 18);
    
    // Convert to double for calculations
    double balanceDbl = atof(balanceStr.c_str());
    
    return balanceDbl;
}

/**
 * Query current staking status for an address
 * Shows: current stake amount and mining reward multiplier
 */
void queryStakingStatus(const char *address)
{
    Serial.println("--- Querying Staking Status ---");
    
    string addr = address;
    string stakingContract = APLO_STAKING_CONTRACT;
    
    // Get current stake amount (in Gaplo/wei)
    uint256_t stakeGaplo = web3->AploGetStake(&stakingContract, &addr);
    
    // Convert to APLO for display
    string stakeStr = Util::ConvertWeiToEthString(&stakeGaplo, 18);
    double stakeDbl = atof(stakeStr.c_str());
    
    Serial.print("Address: ");
    Serial.println(address);
    Serial.print("Current Stake: ");
    Serial.print(stakeStr.c_str());
    Serial.println(" APLO");
    
    // Get mining reward multiplier (scaled by 10: 10 = 1.0×, 17 = 1.7×)
    uint256_t multiplierScaled = web3->AploGetStakeMultiplier(&stakingContract, &addr);
    int multiplierInt = (int)multiplierScaled;
    
    Serial.print("Mining Multiplier: ");
    if (multiplierInt == 0) {
        Serial.println("0 (no rewards - stake below 1,000 APLO)");
    } else {
        // Convert scaled multiplier to decimal (e.g., 10 → 1.0, 17 → 1.7)
        double multiplier = multiplierInt / 10.0;
        Serial.print(multiplier, 1);
        Serial.print("× (");
        Serial.print(multiplierInt);
        Serial.println("/10)");
    }
    
    // Show tier information
    Serial.println("\nTier Information:");
    if (stakeDbl < 1000.0) {
        Serial.println("  Below minimum stake (1,000 APLO)");
        Serial.println("  No mining rewards");
    } else if (stakeDbl < 2000.0) {
        Serial.println("  Tier 1: 1,000-1,999 APLO → 1.0× multiplier");
    } else if (stakeDbl < 3000.0) {
        Serial.println("  Tier 2: 2,000-2,999 APLO → 1.1× multiplier");
    } else if (stakeDbl < 4000.0) {
        Serial.println("  Tier 3: 3,000-3,999 APLO → 1.2× multiplier");
    } else if (stakeDbl < 5000.0) {
        Serial.println("  Tier 4: 4,000-4,999 APLO → 1.3× multiplier");
    } else if (stakeDbl < 6000.0) {
        Serial.println("  Tier 5: 5,000-5,999 APLO → 1.4× multiplier");
    } else if (stakeDbl < 7000.0) {
        Serial.println("  Tier 6: 6,000-6,999 APLO → 1.5× multiplier");
    } else if (stakeDbl < 8000.0) {
        Serial.println("  Tier 7: 7,000-7,999 APLO → 1.6× multiplier");
    } else {
        Serial.println("  Tier 8 (MAX): 8,000+ APLO → 1.7× multiplier");
    }
    
    Serial.println();
}

/**
 * Stake APLO in the staking contract
 * Locks the specified amount and increases mining multiplier
 * 
 * @param aplo Amount in APLO to stake (minimum 1,000 for rewards)
 */
void stakeAplo(double aplo)
{
    Serial.println("--- Preparing Staking Transaction ---\n");
    
    // Convert APLO to Gaplo (wei) - 18 decimals
    uint256_t valueGaplo = Util::ConvertToWei(aplo, 18);
    
    Serial.print("Stake Amount: ");
    Serial.print(aplo, 2);
    Serial.println(" APLO");
    Serial.print("Amount in Gaplo (wei): ");
    Serial.println(valueGaplo.str().c_str());
    Serial.println();
    
    Serial.println("Transaction Parameters:");
    Serial.print("  From: ");
    Serial.println(MY_ADDRESS);
    Serial.print("  To (Contract): ");
    Serial.println(APLO_STAKING_CONTRACT);
    Serial.print("  Value: ");
    Serial.print(aplo, 2);
    Serial.println(" APLO (sent WITH transaction)");
    Serial.println();
    
    // Call Web3::AploStake helper
    // This handles nonce retrieval, gas price, function encoding, signing, and submission
    Serial.println("Signing and sending staking transaction...");
    string stakingContract = APLO_STAKING_CONTRACT;
    string myAddr = MY_ADDRESS;
    string txHash = web3->AploStake(&stakingContract, &valueGaplo, PRIVATE_KEY, &myAddr);
    
    Serial.println("\n--- Staking Transaction Result ---\n");
    
    if (txHash.length() > 0 && txHash != "0x") 
    {
        Serial.println("SUCCESS! Staking transaction sent.");
        Serial.print("Transaction Hash: ");
        Serial.println(txHash.c_str());
        Serial.println();
        Serial.println("Your APLO is now staked in the contract.");
        Serial.println("Mining multiplier will be updated based on total stake.");
        Serial.println("Use unstake() to retrieve your staked APLO.");
    }
    else
    {
        Serial.println("ERROR: Staking transaction failed!");
        Serial.println("Possible reasons:");
        Serial.println("  - Insufficient balance for stake amount + gas");
        Serial.println("  - Invalid contract address");
        Serial.println("  - Network connectivity issues");
        Serial.println("  - RPC endpoint unavailable");
    }
    
    Serial.println();
}

/**
 * Unstake ALL staked APLO from the staking contract
 * Returns entire stake to the caller and resets multiplier to 0
 * 
 * WARNING: This unstakes ALL your APLO, not a partial amount
 */
void unstakeAplo()
{
    Serial.println("--- Preparing Unstaking Transaction ---\n");
    
    Serial.println("Transaction Parameters:");
    Serial.print("  From: ");
    Serial.println(MY_ADDRESS);
    Serial.print("  To (Contract): ");
    Serial.println(APLO_STAKING_CONTRACT);
    Serial.println("  Value: 0 APLO (unstake returns staked APLO)");
    Serial.println();
    
    // Call Web3::AploUnstake helper
    // This handles nonce retrieval, gas price, function encoding, signing, and submission
    Serial.println("Signing and sending unstaking transaction...");
    string stakingContract = APLO_STAKING_CONTRACT;
    string myAddr = MY_ADDRESS;
    string txHash = web3->AploUnstake(&stakingContract, PRIVATE_KEY, &myAddr);
    
    Serial.println("\n--- Unstaking Transaction Result ---\n");
    
    if (txHash.length() > 0 && txHash != "0x") 
    {
        Serial.println("SUCCESS! Unstaking transaction sent.");
        Serial.print("Transaction Hash: ");
        Serial.println(txHash.c_str());
        Serial.println();
        Serial.println("Your staked APLO will be returned to your address.");
        Serial.println("Mining multiplier will be reset to 0.");
        Serial.println("You must stake again to receive mining rewards.");
    }
    else
    {
        Serial.println("ERROR: Unstaking transaction failed!");
        Serial.println("Possible reasons:");
        Serial.println("  - No APLO currently staked");
        Serial.println("  - Insufficient gas");
        Serial.println("  - Network connectivity issues");
        Serial.println("  - RPC endpoint unavailable");
    }
    
    Serial.println();
}

/**
 * WiFi setup routine for ESP32
 * Adjust as needed for your platform (ESP8266, etc.)
 */
void setup_wifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
    }

    wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 10)
    {
        for (int i = 0; i < 500; i++)
        {
            delay(1);
        }
        Serial.print(".");
        wificounter++;
    }

    if (wificounter >= 10)
    {
        Serial.println("\nWiFi connection failed. Restarting...");
        ESP.restart();
    }

    delay(10);

    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}
