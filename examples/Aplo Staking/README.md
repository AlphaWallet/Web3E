# AploEmbed Staking Example

This example demonstrates how to interact with the AploCoin staking contract to:
- Query current stake amount and mining reward multiplier
- Stake APLO to increase mining rewards
- Unstake APLO to retrieve locked funds

## Overview

AploCoin uses a built-in staking contract at address `0x0000000000000000000000000000000000001235` that allows miners to lock APLO tokens to increase their mining reward multiplier.

### Staking Tiers

The staking system uses a tier-based multiplier system:

| Staked Amount | Multiplier | Mining Rewards |
|---------------|------------|----------------|
| < 1,000 APLO  | 0×         | No rewards     |
| 1,000-1,999 APLO | 1.0×    | Base rewards   |
| 2,000-2,999 APLO | 1.1×    | +10% rewards   |
| 3,000-3,999 APLO | 1.2×    | +20% rewards   |
| 4,000-4,999 APLO | 1.3×    | +30% rewards   |
| 5,000-5,999 APLO | 1.4×    | +40% rewards   |
| 6,000-6,999 APLO | 1.5×    | +50% rewards   |
| 7,000-7,999 APLO | 1.6×    | +60% rewards   |
| 8,000+ APLO   | 1.7×       | +70% rewards (max) |

**Important Notes:**
- Minimum stake for rewards: **1,000 APLO**
- Staking is **cumulative** - calling `stake()` multiple times adds to your total
- Unstaking returns **ALL** staked APLO and resets multiplier to 0
- Staked APLO is locked in the contract until you unstake

## Hardware Requirements

- ESP32 development board (or compatible Arduino board with WiFi)
- USB cable for programming and serial monitoring
- Stable WiFi connection

## Software Requirements

- PlatformIO IDE or Arduino IDE
- AploEmbed library (this repository)
- ESP32 board support package

## Configuration

### 1. WiFi Credentials

Edit `main.cpp` and update:

```cpp
const char *ssid = "<YOUR_SSID>";
const char *password = "<YOUR_WIFI_PASSWORD>";
```

### 2. Wallet Configuration

**CRITICAL SECURITY STEP:**

Set your actual wallet details before sending transactions:

```cpp
#define MY_ADDRESS "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb"
const char *PRIVATE_KEY = "0000000000000000000000000000000000000000000000000000000000000000";
```

**⚠️ SECURITY WARNING:**
- NEVER commit your real private key to version control
- Keep your private key secret and secure
- Consider using environment variables or secure storage for production

### 3. Staking Amount

Adjust the stake amount (minimum 1,000 APLO for rewards):

```cpp
#define STAKE_AMOUNT_APLO 1000.0
```

### 4. RPC Configuration

The example uses default AploCoin RPC endpoints:
- Primary: `pub1.aplocoin.com`
- Fallback: `pub2.aplocoin.com`

To use custom RPC endpoints, modify the Web3 initialization in `setup()`:

```cpp
// Custom single endpoint
web3 = new Web3("custom-rpc.aplocoin.com");

// Custom primary + fallback
web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
```

## Building and Uploading

### Using PlatformIO

```bash
# Navigate to the example directory
cd "examples/Aplo Staking"

# Build the project
pio run

# Upload to board
pio run --target upload

# Monitor serial output
pio device monitor
```

### Using Arduino IDE

1. Open `main.cpp` in Arduino IDE
2. Select your board: Tools → Board → ESP32 Dev Module
3. Select the correct port: Tools → Port
4. Click Upload
5. Open Serial Monitor (115200 baud)

## Expected Output

```
=== AploEmbed Staking Example ===

Connecting to WiFi: YourNetwork
...
WiFi connected!
IP address: 192.168.1.100

Web3 initialized with AploCoin RPC endpoints
Primary: pub1.aplocoin.com
Fallback: pub2.aplocoin.com

Staking Contract: 0x0000000000000000000000000000000000001235

My Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Current Balance: 5000.000000 APLO

=== Current Staking Status ===

--- Querying Staking Status ---
Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Current Stake: 0.000000000000000000 APLO
Mining Multiplier: 0 (no rewards - stake below 1,000 APLO)

Tier Information:
  Below minimum stake (1,000 APLO)
  No mining rewards

Attempting to stake: 1000.00 APLO

--- Preparing Staking Transaction ---

Stake Amount: 1000.00 APLO
Amount in Gaplo (wei): 1000000000000000000000

Function: stake(uint256)
Call Data: 0xa694fc3a0000000000000000000000000000000000000000000000056bc75e2d63100000

Nonce: 5

Transaction Parameters:
  From: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
  To (Contract): 0x0000000000000000000000000000000000001235
  Value: 1000.00 APLO (sent WITH transaction)
  Gas Price: 1 Gwei
  Gas Limit: 100000
  Nonce: 5

Signing and sending staking transaction...

--- Staking Transaction Result ---

SUCCESS! Staking transaction sent.
Transaction Hash: 0xabcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890

Your APLO is now staked in the contract.
Mining multiplier will be updated based on total stake.
Use unstake() to retrieve your staked APLO.

=== Updated Staking Status ===

--- Querying Staking Status ---
Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Current Stake: 1000.000000000000000000 APLO
Mining Multiplier: 1.0× (10/10)

Tier Information:
  Tier 1: 1,000-1,999 APLO → 1.0× multiplier
```

## API Reference

### Staking Functions

#### `queryStakingStatus(address)`
Queries and displays current stake amount and mining multiplier for an address.

**Parameters:**
- `address` - Wallet address to query (const char*)

**Returns:** void (prints to Serial)

**Example:**
```cpp
queryStakingStatus(MY_ADDRESS);
```

#### `stakeAplo(amount)`
Stakes APLO tokens in the staking contract to increase mining multiplier.

**Parameters:**
- `amount` - Amount in APLO to stake (double)

**Returns:** void (prints transaction result to Serial)

**Notes:**
- Minimum 1,000 APLO required for mining rewards
- Staking is cumulative - multiple calls add to total stake
- Requires sufficient balance for stake amount + gas fees

**Example:**
```cpp
stakeAplo(1000.0);  // Stake 1,000 APLO
```

#### `unstakeAplo()`
Unstakes ALL staked APLO and returns it to the caller's address.

**Parameters:** None

**Returns:** void (prints transaction result to Serial)

**Notes:**
- Returns **entire** stake, not partial amounts
- Resets mining multiplier to 0
- Must stake again to receive mining rewards

**Example:**
```cpp
unstakeAplo();  // Retrieve all staked APLO
```

### Web3 Staking Methods

#### `web3->AploGetStake(contract, address)`
Returns the current stake amount for an address.

**Parameters:**
- `contract` - Staking contract address (string*)
- `address` - Wallet address to query (string*)

**Returns:** `uint256_t` - Stake amount in Gaplo (wei)

**Example:**
```cpp
string stakingContract = APLO_STAKING_CONTRACT;
string addr = MY_ADDRESS;
uint256_t stakeGaplo = web3->AploGetStake(&stakingContract, &addr);
```

#### `web3->AploGetStakeMultiplier(contract, address)`
Returns the mining reward multiplier for an address.

**Parameters:**
- `contract` - Staking contract address (string*)
- `address` - Wallet address to query (string*)

**Returns:** `uint256_t` - Multiplier scaled by 10 (10 = 1.0×, 17 = 1.7×)

**Example:**
```cpp
string stakingContract = APLO_STAKING_CONTRACT;
string addr = MY_ADDRESS;
uint256_t multiplier = web3->AploGetStakeMultiplier(&stakingContract, &addr);
// multiplier = 10 means 1.0×, 17 means 1.7×
```

### Web3 Staking Methods

#### `web3->AploStake(stakingContract, amount, privateKey, fromAddress)`
Stakes APLO in the staking contract to increase mining multiplier.

**Parameters:**
- `stakingContract` - Staking contract address (const string*)
- `amount` - Amount to stake in Gaplo/wei (const uint256_t*)
- `privateKey` - 64-character hex private key (const char*)
- `fromAddress` - Sender address (const string*)

**Returns:** `string` - Transaction hash on success, empty string on failure

**Example:**
```cpp
string stakingContract = APLO_STAKING_CONTRACT;
uint256_t amount = Util::ConvertToWei(1000.0, 18);  // 1000 APLO
string myAddr = MY_ADDRESS;
string txHash = web3->AploStake(&stakingContract, &amount, PRIVATE_KEY, &myAddr);
```

#### `web3->AploUnstake(stakingContract, privateKey, fromAddress)`
Unstakes ALL staked APLO and resets mining multiplier to 0.

**Parameters:**
- `stakingContract` - Staking contract address (const string*)
- `privateKey` - 64-character hex private key (const char*)
- `fromAddress` - Sender address (const string*)

**Returns:** `string` - Transaction hash on success, empty string on failure

**Example:**
```cpp
string stakingContract = APLO_STAKING_CONTRACT;
string myAddr = MY_ADDRESS;
string txHash = web3->AploUnstake(&stakingContract, PRIVATE_KEY, &myAddr);
```

**Note:** Both methods automatically handle:
- Nonce retrieval via `EthGetTransactionCount()`
- Gas price query via `EthGasPrice()`
- Function encoding and ABI packing
- Transaction signing and submission

## Units and Conversions

AploCoin uses 18 decimal places:

- **1 APLO** = 1,000,000,000,000,000,000 Gaplo (10^18)
- **Gaplo** is the smallest unit

### Conversion Helpers

```cpp
// APLO to Gaplo (wei)
uint256_t gaplo = Util::ConvertToWei(1000.0, 18);  // 1000 APLO → Gaplo

// Gaplo (wei) to APLO string
string aplo = Util::ConvertWeiToEthString(&gaplo, 18);  // Gaplo → "1000.0"
```

## Troubleshooting

### Transaction Fails

**Symptom:** Transaction returns empty hash or "0x"

**Possible Causes:**
1. Insufficient balance for stake amount + gas fees
2. Invalid private key
3. Network connectivity issues
4. RPC endpoint unavailable

**Solutions:**
- Check balance with `queryBalance(MY_ADDRESS)`
- Verify private key is correct (64 hex characters, no 0x prefix)
- Test WiFi connection
- Try alternative RPC endpoint

### "Insufficient Balance" Error

**Symptom:** Error message about insufficient balance

**Cause:** Account balance is less than stake amount + gas buffer

**Solution:**
- Reduce `STAKE_AMOUNT_APLO`
- Add more APLO to your wallet
- Check current balance with balance query example

### Multiplier Shows 0 After Staking

**Symptom:** Mining multiplier remains 0 after successful stake

**Possible Causes:**
1. Staked amount is below 1,000 APLO minimum
2. Transaction not yet confirmed
3. Query executed before transaction confirmed

**Solutions:**
- Ensure stake amount ≥ 1,000 APLO
- Wait a few seconds and query again
- Check transaction hash on block explorer

### Unstake Fails with "Nothing Staked"

**Symptom:** Unstake transaction reverts with error

**Cause:** No APLO currently staked for this address

**Solution:**
- Query staking status first with `queryStakingStatus()`
- Verify you're using the correct address
- Stake APLO before attempting to unstake

### WiFi Connection Fails

**Symptom:** "WiFi connection failed. Restarting..."

**Solutions:**
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure 2.4GHz WiFi is available (ESP32 doesn't support 5GHz)
- Try increasing timeout in `setup_wifi()` (change `wificounter < 10` to higher value)

### Compilation Errors

**Symptom:** Build fails with missing includes or undefined references

**Solutions:**
- Ensure AploEmbed library is in your library path
- Check that all required files are present in `src/` directory
- Verify PlatformIO or Arduino IDE is properly configured for ESP32

## Advanced Usage

### Testing Unstake

The example includes commented-out code to test unstaking. To enable:

```cpp
// In setup(), after staking:
// Uncomment these lines:
Serial.println("\n=== Testing Unstake ===\n");
unstakeAplo();
Serial.println("\n=== Final Staking Status ===\n");
queryStakingStatus(MY_ADDRESS);
```

### Cumulative Staking

To test cumulative staking (multiple stake calls):

```cpp
// Stake 1,000 APLO
stakeAplo(1000.0);
queryStakingStatus(MY_ADDRESS);  // Shows 1,000 APLO, 1.0× multiplier

// Stake another 1,000 APLO
stakeAplo(1000.0);
queryStakingStatus(MY_ADDRESS);  // Shows 2,000 APLO, 1.1× multiplier
```

### Periodic Status Monitoring

To monitor staking status periodically, modify `loop()`:

```cpp
void loop() 
{
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    
    // Check every 60 seconds
    if (now - lastCheck > 60000) {
        Serial.println("\n=== Periodic Status Check ===\n");
        queryStakingStatus(MY_ADDRESS);
        lastCheck = now;
    }
    
    delay(1000);
}
```

## Security Best Practices

1. **Never hardcode real private keys** in production code
2. **Use test amounts** when experimenting
3. **Verify contract addresses** before sending transactions
4. **Keep private keys secure** - use hardware wallets or secure key storage
5. **Test on development networks** before using mainnet
6. **Monitor gas prices** to avoid overpaying for transactions
7. **Understand staking mechanics** - staked APLO is locked until unstaked

## Related Examples

- **Aplo Balance Query**: Query APLO/Gaplo balances
- **Aplo Send Transaction**: Send APLO to addresses
- **Aplo Mining** (future): Mining operations with staking multipliers

## References

- AploNode staking implementation: `/home/hermes/projects/aplosdk/aplonode/builtin/aplo/aplo.go`
- Staking contract address: `0x0000000000000000000000000000000000001235`
- Function signatures:
  - `stake(uint256)` - Lock APLO and increase multiplier
  - `unstake()` - Return all staked APLO
  - `getStake(address)` - Query stake amount (view)
  - `getMultiplier(address)` - Query mining multiplier (view)

## License

This example is part of the AploEmbed library.
Based on the original work by James Brown (@JamesSmartCell, @AlphaWallet).
