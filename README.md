# AploEmbed - AploCoin for Embedded Devices

AploEmbed is a specialized blockchain library for ESP32 and Arduino devices, providing native access to the AploCoin blockchain. AploEmbed focuses exclusively on AploCoin operations including balance queries, token transfers, staking, and mining.

## Features

- **Single-Chain Focus**: Optimized exclusively for AploCoin (Chain ID 28282)
- **RPC Failover**: Automatic failover between primary and backup RPC endpoints
- **Complete API**: Balance queries, transactions, staking, and mining operations
- **Unit Conversion**: Built-in helpers for APLO ↔ Gaplo (wei) conversion
- **Embedded-Optimized**: Minimal memory footprint, tested on ESP32
- **Production-Ready Crypto**: Uses Trezor's battle-tested cryptography library

## Installation

### PlatformIO (Recommended)

Add AploEmbed to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200

lib_ldf_mode = deep
lib_deps =
  https://github.com/AploCoin/AploEmbed.git
```

**Note:** You may need `lib_ldf_mode = deep` to locate ESP32 libraries like `EEPROM.h`. If using many libraries, add `board_build.partitions = no_ota.csv` to fit your firmware.

### Arduino IDE

1. Download the repository as a ZIP file
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

## Quick Start

### Basic Setup

```cpp
#include <Web3.h>
#include <AploContracts.h>

// Use default AploCoin RPC endpoints
// Primary: pub1.aplocoin.com
// Fallback: pub2.aplocoin.com
Web3 *web3 = new Web3();

// Or specify custom RPC endpoint
Web3 *web3 = new Web3("custom-rpc.aplocoin.com");

// Or specify both primary and fallback
Web3 *web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
```

**Important:** The default RPC endpoints (pub1.aplocoin.com and pub2.aplocoin.com) are configurable defaults for your application. Do not spam these endpoints with test traffic. Configure your own RPC infrastructure for development and testing.

### Query APLO Balance

```cpp
string address = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";

// Get balance in Gaplo (wei) - raw blockchain units
uint256_t balanceGaplo = web3->AploGetBalance(&address);

// Get balance as human-readable APLO string (18 decimals)
// 1 APLO = 10^18 Gaplo
string balanceAplo = web3->AploGetBalanceInAplo(&address);
Serial.print("Balance: ");
Serial.print(balanceAplo.c_str());
Serial.println(" APLO");
```

### Send APLO Transaction

```cpp
#include <Contract.h>

Contract contract(web3, "");
contract.SetPrivateKey(PRIVATE_KEY);

string toAddress = "0xRecipientAddressHere";
uint256_t amount = Util::ConvertToWei(10.0, 18); // Send 10 APLO

uint32_t nonce = (uint32_t)web3->EthGetTransactionCount(&myAddress);
unsigned long long gasPrice = 1000000000ULL;
uint32_t gasLimit = 21000;
string emptyData = "";

string txHash = contract.SendTransaction(nonce, gasPrice, gasLimit, 
                                         &toAddress, &amount, &emptyData);
Serial.print("Transaction Hash: ");
Serial.println(txHash.c_str());
```

### Check Staking Status

```cpp
#include <AploContracts.h>

string stakingContract = getAploStakingContract();
string myAddress = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";

// Get current stake amount in Gaplo (wei)
uint256_t stakeAmount = web3->AploGetStake(&stakingContract, &myAddress);

// Convert to APLO for display
string stakeAplo = Util::ConvertWeiToEthString(&stakeAmount, 18);
Serial.print("Staked: ");
Serial.print(stakeAplo.c_str());
Serial.println(" APLO");

// Get staking multiplier (affects mining rewards)
uint256_t multiplier = web3->AploGetStakeMultiplier(&stakingContract, &myAddress);
Serial.print("Multiplier: ");
Serial.println(multiplier.str().c_str());
```

### Stake APLO

```cpp
#include <Contract.h>
#include <AploContracts.h>

string stakingContract = getAploStakingContract();
Contract contract(web3, stakingContract.c_str());
contract.SetPrivateKey(PRIVATE_KEY);

// Stake 1000 APLO (minimum for rewards)
uint256_t stakeAmount = Util::ConvertToWei(1000, 18);

// Build stake(uint256) function call
string functionData = contract.SetupContractData("stake(uint256)", &stakeAmount);

// Get nonce and gas parameters
uint32_t nonce = (uint32_t)web3->EthGetTransactionCount(&myAddress);
unsigned long long gasPrice = 1000000000ULL;
uint32_t gasLimit = 100000;
string valueStr = "0x00";

// Send transaction
string result = contract.SendTransaction(nonce, gasPrice, gasLimit, 
                                        &stakingContract, &valueStr, &functionData);
string txHash = web3->getString(&result);
Serial.print("Stake TX: ");
Serial.println(txHash.c_str());
```

### Mining

```cpp
#include <Contract.h>
#include <AploContracts.h>

string miningContract = getAploMiningContract();
string minerAddress = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";

// Get current mining parameters
uint256_t lastBlock, currentDifficulty, totalMined, prevHash;
bool success = web3->AploGetMinerParams(
    &miningContract, 
    &minerAddress,
    &lastBlock, 
    &currentDifficulty, 
    &totalMined, 
    &prevHash
);

if (success) {
    Serial.print("Last Block: ");
    Serial.println(lastBlock.str().c_str());
    Serial.print("Difficulty: ");
    Serial.println(currentDifficulty.str().c_str());
}

// Generate nonce and submit mining transaction
// (See examples/Aplo Mining for complete mining loop)
```

## Examples

Complete working examples are included in the `examples/` directory:

1. **Aplo Balance Query** - Query balance in APLO and Gaplo with RPC failover
2. **Aplo Send Transaction** - Send APLO to an address with balance verification
3. **Aplo Staking** - Query stake/multiplier, stake and unstake operations
4. **Aplo Mining** - Mining loop with difficulty checking and staking verification

Each example includes comprehensive documentation, security warnings, and troubleshooting guides.

## AploCoin Blockchain

AploEmbed supports one blockchain: **AploCoin**

- **Chain ID**: 28282 (APLO_ID)
- **Default RPC Primary**: pub1.aplocoin.com
- **Default RPC Fallback**: pub2.aplocoin.com
- **Native Currency**: APLO
- **Base Unit**: Gaplo (1 APLO = 10^18 Gaplo)

## API Reference

### Balance Operations

- `uint256_t AploGetBalance(string *address)` - Get balance in Gaplo (wei)
- `string AploGetBalanceInAplo(string *address)` - Get balance as APLO string

### Staking Operations

- `uint256_t AploGetStake(string *contract, string *address)` - Get staked amount
- `uint256_t AploGetStakeMultiplier(string *contract, string *address)` - Get multiplier
- `string AploStake(string *stakingContract, uint256_t *amount, const char *privateKey, string *fromAddress)` - Stake APLO (returns transaction hash)
- `string AploUnstake(string *stakingContract, const char *privateKey, string *fromAddress)` - Unstake APLO (returns transaction hash)

### Mining Operations

- `bool AploGetMinerParams(string *contract, string *address, uint256_t *lastBlock, uint256_t *difficulty, uint256_t *totalMined, uint256_t *prevHash)` - Get mining parameters
- `string AploMine(string *miningContract, string *nonce, const char *privateKey, string *fromAddress)` - Submit mining solution (returns transaction hash)

### Transaction Operations

- `uint32_t EthGetTransactionCount(string *address)` - Get nonce for transactions
- `uint256_t EthBlockNumber()` - Get current block number

**Note:** For sending APLO transactions, use the `Contract` class with `SetPrivateKey()` and `SendTransaction()` as shown in the "Send APLO Transaction" example above.

### RPC Failover

- `string execWithFailover(string *method, string *params)` - Execute RPC call with automatic failover

## Unit Conversion

```cpp
// APLO ↔ Gaplo (wei) conversion
// 1 APLO = 1,000,000,000,000,000,000 Gaplo (10^18)

// Convert APLO to Gaplo (wei)
uint256_t gaplo = Util::ConvertToWei(1.5, 18);  // 1.5 APLO → 1500000000000000000 Gaplo

// Convert Gaplo (wei) to APLO string
string aplo = Util::ConvertWeiToEthString(&gaplo, 18);  // "1.5"
```

## Staking Tiers

Staking APLO increases your mining reward multiplier:

| Stake Amount | Multiplier |
|--------------|------------|
| < 1,000 APLO | 0× (no rewards) |
| 1,000-1,999 APLO | 1.0× |
| 2,000-2,999 APLO | 1.1× |
| 3,000-3,999 APLO | 1.2× |
| 4,000-4,999 APLO | 1.3× |
| 5,000-5,999 APLO | 1.4× |
| 6,000-6,999 APLO | 1.5× |
| 7,000-7,999 APLO | 1.6× |
| 8,000+ APLO | 1.7× |

**Minimum stake for mining rewards: 1,000 APLO**

## Security

### TLS Certificate Validation

AploEmbed does not hardcode AploCoin RPC certificates and does not disable certificate validation automatically. By default it leaves TLS handling to `WiFiClientSecure` and the active Arduino/ESP32 trust configuration.

If your target platform does not provide a usable trust store, configure a CA bundle or a specific CA certificate before making RPC calls:

```cpp
// Option 1: CA Certificate Bundle (recommended for ESP32 PlatformIO projects)
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");
Web3 *web3 = new Web3();
web3->setCertificateBundle(rootca_crt_bundle_start);

// Option 2: Specific CA Certificate
const char* root_ca = "-----BEGIN CERTIFICATE-----\n...";
Web3 *web3 = new Web3();
web3->setCertificate(root_ca);
```

TLS guidance:
- AploEmbed does not hardcode AploCoin RPC certificates.
- Default behavior leaves TLS to `WiFiClientSecure` and the platform trust configuration.
- If your platform lacks a trust store, configure a CA bundle or a specific CA certificate before RPC calls.
- Do not disable TLS validation for wallets that hold real funds.

### Quick Security Checklist

⚠️ **Private Key Safety**
- Never hardcode private keys in production code
- Store private keys securely (EEPROM, secure element, etc.)
- Never commit private keys to version control
- Consider using hardware security modules for high-value operations

⚠️ **Transaction Safety**
- Always verify balance before sending transactions
- Double-check recipient addresses
- Test with small amounts first
- Monitor gas prices to avoid overpaying

⚠️ **RPC Endpoint Safety**
- Do not spam public RPC endpoints with test traffic
- Configure your own RPC infrastructure for development
- Implement rate limiting in production applications
- Use failover endpoints for reliability

⚠️ **TLS Security**
- Enable certificate validation for production (see SECURITY.md)
- Use CA bundle or specific CA certificate
- Never use insecure mode with real funds

## Troubleshooting

### Compilation Issues

**Missing EEPROM.h or other ESP32 libraries:**
Add `lib_ldf_mode = deep` to your `platformio.ini`

**Firmware too large:**
Add `board_build.partitions = no_ota.csv` to your `platformio.ini`

### Runtime Issues

**RPC connection failures:**
- Check WiFi connectivity
- Verify RPC endpoint is accessible
- Check firewall/network restrictions
- Try alternate RPC endpoint

**Transaction failures:**
- Verify sufficient balance (including gas)
- Check nonce is correct
- Verify gas price and gas limit
- Check contract address is correct

**Mining not working:**
- Verify you have staked at least 1,000 APLO
- Check block cooldown (20 blocks between mining attempts)
- Verify difficulty is met by your nonce
- Check miner parameters are current

## Technical Details

### Dependencies

AploEmbed uses the following libraries:

- **Trezor Crypto** - ECDSA sign, recover, verify, keccak256
- **cJSON** - JSON parsing and generation
- **uint256_t** - Lightweight uint256 implementation for embedded devices
- **Mersenne Twister** - Optimal random number generation for embedded

### Compatibility

- **Platforms**: ESP32, ESP8266 (ESP32 recommended)
- **Framework**: Arduino
- **C++ Standard**: C++11
- **Memory**: Optimized for embedded devices with limited RAM

### Architecture

AploEmbed is specialized for AploCoin:

- Fixed Chain ID to 28282 (APLO_ID)
- Simplified RPC configuration to AploCoin endpoints
- Added AploCoin-specific API methods
- Maintained backward compatibility through legacy constructors

## Credits

AploEmbed is based on the work by James Brown and the AlphaWallet team. We are grateful for their excellent embedded blockchain framework.

## License

MIT License - see LICENSE file for details

## Support

- **Issues**: https://github.com/AploCoin/AploEmbed/issues
- **AploCoin**: https://aplocoin.com/
- **Documentation**: See examples/ directory for complete working code

## Version

**Current Version**: 0.0.1

This is an early release focused on core AploCoin operations. Future versions will add additional features and optimizations.
