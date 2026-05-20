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
// 2. Mining requires minimum 1,000 APLO staked to receive rewards
// 3. Mining submits transactions that cost gas - ensure sufficient balance
// 4. Mining difficulty adjusts dynamically based on network conditions
// 5. Block reward cooldown: 20 blocks between successful mines per address
// 6. Test with small amounts and monitor gas costs first
// 7. Higher stake = higher mining multiplier (up to 1.7× at 8,000+ APLO)
//
// Mining Process:
//   1. Query miner_params(address) to get current difficulty and prev_hash
//   2. Generate random nonce and hash it with miner params
//   3. If hash < difficulty, submit mine(nonce) transaction
//   4. Wait for 20+ blocks before next mine attempt
//   5. Rewards are multiplied by staking tier (0× to 1.7×)
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

// Mining parameters
#define BLOCK_COOLDOWN 20          // Minimum blocks between mine attempts
#define HASH_ATTEMPTS_PER_CYCLE 100 // Number of nonces to try before checking block height
#define CYCLE_DELAY_MS 1000        // Delay between hash cycles (ms)

Web3 *web3;
int wificounter = 0;

// Mining state
struct MinerParams {
    uint32_t lastBlock;
    String currentDifficulty;
    uint32_t totalMined;
    String prevHash;
};

void setup_wifi();
void queryStakingStatus(const char *address);
MinerParams getMinerParams(const char *address);
bool attemptMining(const char *address);
String generateRandomNonce();
String hashNonce(const String &nonce, const char *address, const String &difficulty, 
                 const String &prevHash, uint32_t totalMined);
bool submitMineTransaction(const String &nonce);
double queryBalance(const char *address);

void setup() 
{
    Serial.begin(115200);
    Serial.println("\n\n=== AploEmbed Mining Example ===\n");
    
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
    
    Serial.print("Mining Contract: ");
    Serial.println(APLO_MINING_CONTRACT);
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
    
    // CRITICAL: Check staking status BEFORE attempting to mine
    Serial.println("=== Checking Staking Status ===\n");
    queryStakingStatus(MY_ADDRESS);
    
    Serial.println("\n=== Starting Mining Loop ===\n");
    Serial.println("Mining will attempt to find valid nonces and submit transactions.");
    Serial.println("Press RESET to stop.\n");
}

void loop() 
{
    // Attempt mining cycle
    bool mined = attemptMining(MY_ADDRESS);
    
    if (mined) {
        Serial.println("\n✓ Successfully mined and submitted transaction!");
        Serial.println("Waiting for block cooldown before next attempt...\n");
        
        // Update balance after successful mine
        double newBalance = queryBalance(MY_ADDRESS);
        Serial.print("Updated Balance: ");
        Serial.print(newBalance, 6);
        Serial.println(" APLO\n");
    }
    
    // Delay between mining cycles
    delay(CYCLE_DELAY_MS);
}

void setup_wifi() 
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
    }

    wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 20) {
        delay(500);
        Serial.print(".");
        wificounter++;
    }

    if (wificounter >= 20) {
        Serial.println("\nFailed to connect to WiFi!");
        Serial.println("Please check your credentials and try again.");
        while (1) { delay(1000); }
    }

    Serial.println("\n✓ WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

double queryBalance(const char *address) 
{
    string addr = address;
    uint256_t balanceWei = web3->AploGetBalance(&addr);
    
    if (balanceWei == 0) {
        return 0.0;
    }
    
    // Convert wei to APLO (18 decimals)
    string balanceAplo = Util::ConvertWeiToEthString(&balanceWei, 18);
    return atof(balanceAplo.c_str());
}

void queryStakingStatus(const char *address) 
{
    string addr = address;
    string stakingContractAddr = APLO_STAKING_CONTRACT;
    
    // Query stake amount
    uint256_t stakeWei = web3->AploGetStake(&stakingContractAddr, &addr);
    string stakeAplo = Util::ConvertWeiToEthString(&stakeWei, 18);
    double stakeAmount = atof(stakeAplo.c_str());
    
    // Query multiplier (scaled by 10, e.g., 10 = 1.0×, 17 = 1.7×)
    uint256_t multiplierRaw = web3->AploGetStakeMultiplier(&stakingContractAddr, &addr);
    uint32_t multiplierScaled = (uint32_t)multiplierRaw;
    double multiplier = multiplierScaled / 10.0;
    
    Serial.print("Staked Amount: ");
    Serial.print(stakeAmount, 2);
    Serial.println(" APLO");
    
    Serial.print("Mining Multiplier: ");
    Serial.print(multiplier, 1);
    Serial.println("×");
    
    // Determine tier and display status
    if (stakeAmount < 1000.0) {
        Serial.println("\n⚠ WARNING: Stake is below 1,000 APLO");
        Serial.println("Mining will NOT earn rewards at 0× multiplier!");
        Serial.println("Please stake at least 1,000 APLO to receive mining rewards.");
        Serial.println("See 'Aplo Staking' example for staking instructions.\n");
    } else if (stakeAmount < 2000.0) {
        Serial.println("\n✓ Tier 1: Eligible for mining rewards at 1.0× multiplier");
        Serial.println("Stake 2,000+ APLO to increase multiplier to 1.1×\n");
    } else if (stakeAmount < 8000.0) {
        uint32_t tier = (uint32_t)(stakeAmount / 1000.0);
        Serial.print("\n✓ Tier ");
        Serial.print(tier);
        Serial.print(": Mining rewards at ");
        Serial.print(multiplier, 1);
        Serial.println("× multiplier");
        
        if (stakeAmount < 8000.0) {
            uint32_t nextTier = tier + 1;
            double nextMultiplier = (10.0 + nextTier) / 10.0;
            Serial.print("Stake ");
            Serial.print(nextTier * 1000);
            Serial.print("+ APLO to increase multiplier to ");
            Serial.print(nextMultiplier, 1);
            Serial.println("×\n");
        }
    } else {
        Serial.println("\n✓ MAX TIER: Mining rewards at 1.7× multiplier (maximum)");
        Serial.println("You have reached the highest staking tier!\n");
    }
}

MinerParams getMinerParams(const char *address) 
{
    MinerParams params;
    
    // Call miner_params(address) view function using Web3::AploGetMinerParams
    string addr = address;
    string miningContractAddr = APLO_MINING_CONTRACT;
    
    uint256_t lastBlock, currentDifficulty, totalMined, prevHash;
    
    bool success = web3->AploGetMinerParams(&miningContractAddr, &addr,
                                            &lastBlock, &currentDifficulty,
                                            &totalMined, &prevHash);
    
    if (success) {
        params.lastBlock = (uint32_t)lastBlock;
        params.totalMined = (uint32_t)totalMined;
        
        // Convert uint256_t to hex string
        char diffHex[67], prevHashHex[67];
        snprintf(diffHex, sizeof(diffHex), "0x%064llx%064llx", 
                 (unsigned long long)(currentDifficulty >> 64), 
                 (unsigned long long)(currentDifficulty & 0xFFFFFFFFFFFFFFFFULL));
        snprintf(prevHashHex, sizeof(prevHashHex), "0x%064llx%064llx",
                 (unsigned long long)(prevHash >> 64),
                 (unsigned long long)(prevHash & 0xFFFFFFFFFFFFFFFFULL));
        
        params.currentDifficulty = String(diffHex);
        params.prevHash = String(prevHashHex);
    } else {
        // Fallback to safe defaults if RPC call fails
        params.lastBlock = 0;
        params.currentDifficulty = "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
        params.totalMined = 0;
        params.prevHash = "0x0000000000000000000000000000000000000000000000000000000000000000";
    }
    
    Serial.println("Miner Parameters:");
    Serial.print("  Last Block: ");
    Serial.println(params.lastBlock);
    Serial.print("  Total Mined: ");
    Serial.println(params.totalMined);
    Serial.print("  Difficulty: ");
    Serial.println(params.currentDifficulty);
    
    return params;
}

bool attemptMining(const char *address) 
{
    // Get current miner parameters
    MinerParams params = getMinerParams(address);
    
    // Get current block number
    int currentBlock = web3->EthBlockNumber();
    
    Serial.print("Current Block: ");
    Serial.println(currentBlock);
    
    // Check block cooldown
    if (currentBlock - params.lastBlock < BLOCK_COOLDOWN) {
        uint32_t blocksRemaining = BLOCK_COOLDOWN - (currentBlock - params.lastBlock);
        Serial.print("⏳ Cooldown active: ");
        Serial.print(blocksRemaining);
        Serial.println(" blocks remaining");
        return false;
    }
    
    Serial.println("✓ Cooldown complete, attempting to mine...");
    
    // Try multiple nonces
    for (int i = 0; i < HASH_ATTEMPTS_PER_CYCLE; i++) {
        String nonce = generateRandomNonce();
        String hash = hashNonce(nonce, address, params.currentDifficulty, 
                               params.prevHash, params.totalMined);
        
        // Compare hash with difficulty using proper uint256 comparison
        string hashStr = hash.c_str();
        string diffStr = params.currentDifficulty.c_str();
        
        if (Util::CompareUint256(&hashStr, &diffStr)) {
            // Found valid nonce!
            Serial.println("\n\n🎉 VALID NONCE FOUND!");
            Serial.print("Nonce: ");
            Serial.println(nonce);
            Serial.print("Hash: ");
            Serial.println(hash);
            Serial.print("Difficulty: ");
            Serial.println(params.currentDifficulty);
            
            // Submit mining transaction
            return submitMineTransaction(nonce);
        }
        
        if (i % 10 == 0) {
            Serial.print(".");
        }
    }
    
    Serial.println("\nNo valid nonce found in this cycle.");
    return false;
}

String generateRandomNonce() 
{
    // Generate random 32-byte nonce (64 hex characters)
    String nonce = "0x";
    for (int i = 0; i < 64; i++) {
        nonce += String(random(0, 16), HEX);
    }
    return nonce;
}

String hashNonce(const String &nonce, const char *address, const String &difficulty, 
                 const String &prevHash, uint32_t totalMined) 
{
    // Hash = keccak256(abi.encodePacked(address, nonce, difficulty, prevHash, totalMined))
    // This matches the WebMiner implementation
    
    string addr = address;
    string nonceStr = nonce.c_str();
    string diffStr = difficulty.c_str();
    string prevHashStr = prevHash.c_str();
    
    // Pack data according to Solidity abi.encodePacked
    string packed = Util::PackMiningData(&addr, &nonceStr, &diffStr, &prevHashStr, totalMined);
    
    // Compute keccak256 hash
    string hash = Util::ComputeKeccak256(&packed);
    
    return String(hash.c_str());
}

bool submitMineTransaction(const String &nonce) 
{
    Serial.println("\n🔨 Submitting mine transaction...");
    
    // Call Web3::AploMine helper
    // This handles nonce validation, function encoding, nonce retrieval, gas price, signing, and submission
    string miningContractAddr = APLO_MINING_CONTRACT;
    string nonceStr = nonce.c_str();
    string myAddr = MY_ADDRESS;
    string txHash = web3->AploMine(&miningContractAddr, &nonceStr, PRIVATE_KEY, &myAddr);
    
    if (txHash.empty() || txHash == "0x" || txHash == "0x0") {
        Serial.println("✗ Transaction failed!");
        return false;
    }
    
    Serial.print("✓ Transaction submitted: ");
    Serial.println(txHash.c_str());
    
    return true;
}
