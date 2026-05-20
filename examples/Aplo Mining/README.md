# AploEmbed Mining Example

This example demonstrates how to mine AploCoin (APLO) blocks from an ESP32 microcontroller using the AploEmbed library. Mining requires staking at least 1,000 APLO to receive rewards.

## Overview

Mining on AploCoin is a Proof-of-Work process where miners:
1. Query current mining parameters (difficulty, previous hash, total mined)
2. Generate random nonces and hash them with miner parameters
3. Submit valid nonces (hash < difficulty) as mine transactions
4. Receive block rewards multiplied by their staking tier (0× to 1.7×)

**IMPORTANT**: Mining requires minimum 1,000 APLO staked. Without staking, mining multiplier is 0× and no rewards are earned.

## Features Demonstrated

- **Mining Loop**: Continuous mining with difficulty checking
- **Staking Verification**: Check staking status before mining
- **Block Cooldown**: Respect 20-block cooldown between mine attempts
- **Miner Parameters**: Query `miner_params(address)` for current state
- **Nonce Generation**: Random nonce generation and hashing
- **Transaction Submission**: Submit `mine(bytes32)` transactions
- **Multiplier Display**: Show current mining reward multiplier based on stake

## Hardware Requirements

- ESP32 development board (ESP32-WROOM-32 or similar)
- WiFi network connection
- Sufficient APLO balance for gas fees (mining transactions cost gas)
- Minimum 1,000 APLO staked for rewards (see Aplo Staking example)

## Software Requirements

- PlatformIO IDE or Arduino IDE with ESP32 support
- AploEmbed library (this repository)
- WiFi credentials for network access

## Configuration

### 1. WiFi Setup

Edit `main.cpp` and replace WiFi credentials:

```cpp
const char *ssid = "YourNetworkName";
const char *password = "YourWiFiPassword";
```

### 2. Wallet Configuration

**CRITICAL SECURITY**: Replace with your actual wallet address and private key:

```cpp
#define MY_ADDRESS "0xYourWalletAddress"
const char *PRIVATE_KEY = "your64characterhexprivatekeywithout0xprefix";
```

**WARNING**: 
- NEVER commit your real private key to version control
- Keep your private key secret and secure
- Consider using environment variables or secure storage in production

### 3. Mining Parameters (Optional)

Adjust mining behavior if needed:

```cpp
#define BLOCK_COOLDOWN 20          // Blocks between mine attempts (network rule)
#define HASH_ATTEMPTS_PER_CYCLE 100 // Nonces to try per cycle
#define CYCLE_DELAY_MS 1000        // Delay between cycles (ms)
```

## RPC Configuration

The example uses default AploCoin RPC endpoints with automatic failover:

```cpp
// Default: pub1.aplocoin.com (primary), pub2.aplocoin.com (fallback)
web3 = new Web3();

// Custom single endpoint:
web3 = new Web3("custom-rpc.aplocoin.com");

// Custom primary + fallback:
web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
```

## Building and Uploading

### Using PlatformIO

```bash
cd "examples/Aplo Mining"
pio run --target upload
pio device monitor
```

### Using Arduino IDE

1. Open `main.cpp` in Arduino IDE
2. Select board: Tools → Board → ESP32 Arduino → ESP32 Dev Module
3. Select port: Tools → Port → (your ESP32 port)
4. Click Upload
5. Open Serial Monitor (115200 baud)

## Expected Output

```
=== AploEmbed Mining Example ===

Connecting to WiFi: YourNetwork
✓ WiFi connected
IP address: 192.168.1.100

Web3 initialized with AploCoin RPC endpoints
Primary: pub1.aplocoin.com
Fallback: pub2.aplocoin.com

Mining Contract: 0x0000000000000000000000000000000000001234
Staking Contract: 0x0000000000000000000000000000000000001235

My Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Current Balance: 5432.123456 APLO

=== Checking Staking Status ===

Staked Amount: 3000.00 APLO
Mining Multiplier: 1.2×

✓ Tier 3: Mining rewards at 1.2× multiplier
Stake 4000+ APLO to increase multiplier to 1.3×

=== Starting Mining Loop ===

Mining will attempt to find valid nonces and submit transactions.
Press RESET to stop.

Miner Parameters:
  Last Block: 12345
  Total Mined: 42
  Difficulty: 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
Current Block: 12370
✓ Cooldown complete, attempting to mine...
..........
No valid nonce found in this cycle.

[Mining continues...]
```

## Mining Process Explained

### 1. Staking Check

Before mining, the example queries your staking status:

```cpp
queryStakingStatus(MY_ADDRESS);
```

This shows:
- Current staked amount in APLO
- Mining reward multiplier (0× to 1.7×)
- Staking tier and next tier requirements

**If stake < 1,000 APLO**: Warning displayed, mining will not earn rewards.

### 2. Miner Parameters

The mining contract tracks per-address state via `miner_params(address)`:

```solidity
struct MinerParams {
    uint256 last_block;        // Last block where this address mined
    uint256 current_difficulty; // Current mining difficulty
    uint256 total_mined;       // Total blocks mined by this address
    uint256 prev_hash;         // Previous hash for nonce validation
}
```

### 3. Block Cooldown

AploCoin enforces a 20-block cooldown between successful mines per address. The example checks:

```cpp
if (currentBlock - params.lastBlock < BLOCK_COOLDOWN) {
    // Wait for cooldown to expire
}
```

### 4. Nonce Mining

The mining algorithm:

1. Generate random 32-byte nonce
2. Compute hash = keccak256(address, nonce, difficulty, prevHash, totalMined)
3. If hash < difficulty, submit mine(nonce) transaction
4. Otherwise, try next nonce

```cpp
String hash = hashNonce(nonce, address, difficulty, prevHash, totalMined);
if (hash < difficulty) {
    submitMineTransaction(nonce);
}
```

### 5. Transaction Submission

Valid nonces are submitted via `Web3::AploMine()` helper:

```cpp
string miningContractAddr = APLO_MINING_CONTRACT;
string nonceStr = nonce.c_str();
string myAddr = MY_ADDRESS;
string txHash = web3->AploMine(&miningContractAddr, &nonceStr, PRIVATE_KEY, &myAddr);
```

The helper automatically handles:
- Nonce validation (must be 32 bytes / 64 hex chars)
- Function encoding for `mine(bytes32)`
- Transaction nonce retrieval via `EthGetTransactionCount()`
- Gas price query via `EthGasPrice()`
- Transaction signing and submission

### 6. Reward Calculation

Block rewards are multiplied by staking tier:

- Base reward: Fixed per block (e.g., 50 APLO)
- Actual reward = Base reward × Multiplier
- Example: 50 APLO × 1.2× = 60 APLO

## Staking Tiers and Multipliers

Mining rewards scale with staked amount:

| Staked APLO | Multiplier | Reward Example (50 APLO base) |
|-------------|------------|-------------------------------|
| < 1,000     | 0×         | 0 APLO (no reward)            |
| 1,000-1,999 | 1.0×       | 50 APLO                       |
| 2,000-2,999 | 1.1×       | 55 APLO                       |
| 3,000-3,999 | 1.2×       | 60 APLO                       |
| 4,000-4,999 | 1.3×       | 65 APLO                       |
| 5,000-5,999 | 1.4×       | 70 APLO                       |
| 6,000-6,999 | 1.5×       | 75 APLO                       |
| 7,000-7,999 | 1.6×       | 80 APLO                       |
| 8,000+      | 1.7×       | 85 APLO (maximum)             |

**To stake APLO**, see the `Aplo Staking` example.

## API Reference

### Mining Functions

#### `getMinerParams(address)`
Query current mining state for an address.

**Returns**: `MinerParams` struct with:
- `lastBlock`: Last block where address mined
- `currentDifficulty`: Current mining difficulty target
- `totalMined`: Total blocks mined by address
- `prevHash`: Previous hash for validation

**Example**:
```cpp
MinerParams params = getMinerParams(MY_ADDRESS);
Serial.println(params.totalMined);
```

#### `attemptMining(address)`
Attempt to mine a block by trying multiple nonces.

**Returns**: `true` if valid nonce found and submitted, `false` otherwise

**Example**:
```cpp
bool mined = attemptMining(MY_ADDRESS);
if (mined) {
    Serial.println("Successfully mined!");
}
```

#### `generateRandomNonce()`
Generate a random 32-byte nonce for mining.

**Returns**: Hex string (0x + 64 characters)

**Example**:
```cpp
String nonce = generateRandomNonce();
// "0x1a2b3c4d..."
```

#### `hashNonce(nonce, address, difficulty, prevHash, totalMined)`
Compute mining hash for a nonce.

**Parameters**:
- `nonce`: 32-byte nonce (hex string)
- `address`: Miner address
- `difficulty`: Current difficulty target
- `prevHash`: Previous hash from miner_params
- `totalMined`: Total mined count from miner_params

**Returns**: keccak256 hash (hex string)

**Example**:
```cpp
String hash = hashNonce(nonce, MY_ADDRESS, params.currentDifficulty, 
                       params.prevHash, params.totalMined);
```

#### `submitMineTransaction(nonce)`
Submit a valid nonce as a mine transaction.

**Parameters**:
- `nonce`: Valid 32-byte nonce (hex string)

**Returns**: `true` if transaction submitted successfully

**Example**:
```cpp
if (submitMineTransaction(nonce)) {
    Serial.println("Transaction submitted!");
}
```

### Staking Functions

#### `queryStakingStatus(address)`
Display current staking status and mining multiplier.

**Parameters**:
- `address`: Wallet address to query

**Output**: Prints staked amount, multiplier, tier, and warnings

**Example**:
```cpp
queryStakingStatus(MY_ADDRESS);
// Staked Amount: 3000.00 APLO
// Mining Multiplier: 1.2×
// ✓ Tier 3: Mining rewards at 1.2× multiplier
```

### Balance Functions

#### `queryBalance(address)`
Query APLO balance for an address.

**Parameters**:
- `address`: Wallet address to query

**Returns**: Balance in APLO (double)

**Example**:
```cpp
double balance = queryBalance(MY_ADDRESS);
Serial.print(balance, 6);  // 1234.567890 APLO
```

## Troubleshooting

### "Cooldown active: X blocks remaining"

**Cause**: You recently mined a block and must wait 20 blocks before next attempt.

**Solution**: Wait for the cooldown to expire. This is a network rule to prevent spam.

### "⚠ WARNING: Stake is below 1,000 APLO"

**Cause**: Your staked amount is below the minimum for mining rewards.

**Solution**: Stake at least 1,000 APLO using the `Aplo Staking` example. Mining without staking earns 0 rewards.

### "No valid nonce found in this cycle"

**Cause**: Mining difficulty is high, and no valid nonce was found in this attempt.

**Solution**: This is normal. Mining is probabilistic. The loop will continue trying. Consider:
- Increasing `HASH_ATTEMPTS_PER_CYCLE` for more attempts per cycle
- Reducing `CYCLE_DELAY_MS` for faster cycles
- Note: ESP32 mining is slow compared to GPU/ASIC miners

### "Transaction failed!"

**Possible causes**:
1. Insufficient balance for gas fees
2. Nonce already used (race condition with other miners)
3. Block cooldown not respected (timing issue)
4. RPC endpoint unavailable

**Solutions**:
- Check balance: `queryBalance(MY_ADDRESS)`
- Verify RPC connectivity
- Ensure 20-block cooldown is respected
- Check gas price is sufficient for network

### WiFi connection fails

**Cause**: Incorrect credentials or network issues.

**Solution**:
- Verify SSID and password in `main.cpp`
- Check WiFi signal strength
- Ensure network allows ESP32 connections
- Try 2.4GHz network (ESP32 doesn't support 5GHz)

### "Contract not initialized"

**Cause**: Web3 or Contract objects not properly initialized.

**Solution**:
- Ensure WiFi is connected before Web3 initialization
- Check RPC endpoints are accessible
- Verify contract addresses in `AploContracts.h`

## Units and Conversions

AploCoin uses 18 decimal places:

- **1 APLO** = 1,000,000,000,000,000,000 Gaplo (10^18)
- **1 Gaplo** = 0.000000000000000001 APLO (10^-18)
- **Wei** = Smallest unit (same as Gaplo for AploCoin)

The library handles conversions automatically:

```cpp
// Convert APLO to Gaplo (wei)
String wei = Util::ConvertToWei("1000.5", 18);  // "1000500000000000000000"

// Convert Gaplo (wei) to APLO
String aplo = Util::ConvertWeiToEthString("1000500000000000000000", 18);  // "1000.5"
```

## Security Best Practices

### Private Key Management

1. **NEVER commit private keys to version control**
   - Add `main.cpp` to `.gitignore` if it contains real keys
   - Keep committed examples on deterministic demo keys only; set real keys through secure storage

2. **Use environment variables or secure storage**
   - For production, load keys from secure storage (SPIFFS, EEPROM with encryption)
   - Consider hardware wallets for high-value operations

3. **Limit wallet balance**
   - Keep only necessary APLO for gas fees in mining wallet
   - Transfer rewards to cold storage regularly

### Network Security

1. **Use HTTPS RPC endpoints** (when available)
2. **Verify transaction parameters** before signing
3. **Monitor for unusual activity** (unexpected balance changes)

### Gas Management

1. **Set reasonable gas limits** to avoid overpaying
2. **Monitor gas prices** and adjust based on network conditions
3. **Keep buffer balance** for gas fees (mining transactions cost gas)

## Advanced Usage

### Custom Difficulty Comparison

The library includes proper uint256 difficulty comparison via `Util::CompareUint256()`:

```cpp
// Compare hash < difficulty using lexicographic comparison on padded hex strings
string hashStr = hash.c_str();
string diffStr = difficulty.c_str();
if (Util::CompareUint256(&hashStr, &diffStr)) {
    // Valid nonce found
}
```

### Optimized Hashing

For better mining performance, consider:

```cpp
// Hardware acceleration options:
// - ESP32 has hardware SHA acceleration, but not keccak256
// - Software keccak256 is already optimized in the library
// - For serious mining, use dedicated hardware (ASIC/GPU)

// Multi-core optimization (ESP32 dual-core):
// - Run mining loop on Core 0
// - Handle WiFi/RPC on Core 1
// - Use FreeRTOS tasks for parallel nonce generation
```

### Mining Pool Support

To join a mining pool, implement pool protocol:

```cpp
// Pool mining workflow:
// 1. Connect to pool server via WebSocket/HTTP
// 2. Receive work assignments (difficulty, prev_hash)
// 3. Submit shares (partial proofs below network difficulty)
// 4. Receive proportional rewards based on shares contributed
// Note: Pool protocol is pool-specific and not standardized
```

## Performance Considerations

### ESP32 Mining Performance

**Reality check**: ESP32 mining is **extremely slow** compared to dedicated hardware:

- ESP32: ~1-10 hashes/second
- Modern GPU: ~1,000,000+ hashes/second
- ASIC miner: ~1,000,000,000+ hashes/second

**ESP32 mining is for**:
- Educational purposes
- Demonstration of blockchain concepts
- Low-difficulty test networks
- Participation in community mining (not profit)

**Not recommended for**:
- Profit-driven mining (electricity cost > rewards)
- High-difficulty networks
- Production mining operations

### Optimization Tips

If you want to maximize ESP32 mining performance:

1. **Increase hash attempts per cycle**:
   ```cpp
   #define HASH_ATTEMPTS_PER_CYCLE 1000  // More attempts
   ```

2. **Reduce cycle delay**:
   ```cpp
   #define CYCLE_DELAY_MS 100  // Faster cycles
   ```

3. **Use both ESP32 cores**:
   ```cpp
   // Multi-core mining example using FreeRTOS tasks
   xTaskCreatePinnedToCore(miningTask, "Mining", 4096, NULL, 1, NULL, 0);
   xTaskCreatePinnedToCore(miningTask, "Mining", 4096, NULL, 1, NULL, 1);
   // Note: Requires careful synchronization of shared state
   ```

4. **Optimize keccak256 implementation** (when implemented)

## Related Examples

- **Aplo Balance Query**: Query APLO/Gaplo balances
- **Aplo Send Transaction**: Send APLO to addresses
- **Aplo Staking**: Stake APLO to increase mining multiplier (REQUIRED for mining rewards)

## Contract Addresses

Mining and staking contracts (from `AploContracts.h`):

```cpp
#define APLO_MINING_CONTRACT  "0x0000000000000000000000000000000000001234"
#define APLO_STAKING_CONTRACT "0x0000000000000000000000000000000000001235"
```

These are example addresses from the WebMiner reference. Update with actual deployed addresses for your network.

## References

- **WebMiner**: `/home/hermes/projects/aplosdk/webminer/src/components/webMiner.tsx`
- **AploNode**: `/home/hermes/projects/aplosdk/aplonode` (RPC implementation)
- **Mining Contract ABI**: See WebMiner `CONTRACT_ABI` for full interface
- **Staking Contract ABI**: See WebMiner `APLO_STAKING_ABI` for full interface

## License

Same as AploEmbed library (check repository root for license details).

## Support

For issues or questions:
1. Check this README's Troubleshooting section
2. Review WebMiner reference implementation
3. Consult AploNode RPC documentation
4. Open an issue in the AploEmbed repository

---

**Happy Mining! ⛏️**

Remember: Stake at least 1,000 APLO before mining to receive rewards!
