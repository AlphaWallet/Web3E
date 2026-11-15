# Web3E Test Suite

PlatformIO Unity test suite for Web3E library on ESP32.

## Test Structure

- **`test_crypto/`** - Cryptography tests (Keccak-256, key derivation, signing, recovery)
- **`test_util/`** - Utility function tests (hex conversion, Wei/Eth conversion, uint256, RLP)
- **`test_transaction/`** - Transaction signing and ABI encoding tests (offline)
- **`test_network_rpc/`** - Network RPC tests (read-only, no funds needed):
  - Wi-Fi connection using WiFiConnection class
  - Chain ID, block number queries
  - Native ETH balance queries
  - Transaction count (nonce) queries
  - ERC20 token balance, name, symbol queries
- **`test_network_tx/`** - Network transaction tests (SENDS REAL TRANSACTIONS):
  - Transaction signing (dry-run)
  - Send ETH transaction and verify
  - Send ERC20 token transaction and verify
  - Requires funded Sepolia testnet account

## Running Tests

### Offline Tests (Default)

Offline tests run without Wi-Fi or RPC connectivity. They test pure cryptographic and utility functions.

**Compile only (CI):**
```bash
pio test -e esp32dev --without-uploading --without-testing
```

**Run on connected ESP32 board:**
```bash
pio test -e esp32dev --upload-port COM15 --test-port COM15
```

Replace `COM15` with your board's serial port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux, `/dev/cu.usbserial-*` on macOS).

### Network Tests (Optional)

Network tests require Wi-Fi credentials and an Ethereum RPC endpoint.

**Setup:**

1. Copy the example config:
   ```bash
   cp test_config.ini.example test_config.ini
   ```

2. Edit `test_config.ini` with your credentials:
   ```ini
   # Wi-Fi credentials
   WIFI_SSID=YourNetworkName
   WIFI_PASS=YourWiFiPassword
   
   # RPC endpoint (Sepolia testnet recommended)
   RPC_URL=https://sepolia.infura.io/v3/your-api-key
   
   # Address for balance queries (defaults to Vitalik's address)
   TEST_ADDRESS=0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045
   
   # ERC20 contract for token queries (e.g. USDC on Sepolia)
   ERC20_CONTRACT=0x1c7D4B196Cb0C7B01d743Fbc6116a902379C7238
   
   # Optional: For transaction tests (TESTNET ONLY - requires funded account)
   TEST_PRIVATE_KEY=
   TEST_TO_ADDRESS=
   ```
   
   **Security Warning**: Never use mainnet private keys in tests! Generate a dedicated testnet key for testing.

3. Run network tests:
   ```bash
   pio test -e esp32dev_network --upload-port COM15 --test-port COM15
   ```

**Note:** `test_config.ini` is in `.gitignore` and will never be committed.

**Alternative: Environment Variables**

You can also pass credentials via environment variables:

**Windows:**
```cmd
set WIFI_SSID=YourSSID
set WIFI_PASS=YourPassword
set RPC_URL=https://sepolia.infura.io/v3/your-key
pio test -e esp32dev_network --upload-port COM15 --test-port COM15
```

**Linux/macOS:**
```bash
WIFI_SSID=YourSSID WIFI_PASS=YourPassword RPC_URL=https://... pio test -e esp32dev_network --upload-port COM15 --test-port COM15
```

## Adding New Tests

### Offline Tests

Create a new directory under `test/` with a descriptive name:

```
test/test_myfeature/
  test_myfeature.cpp
```

Structure your test file:

```cpp
#include <Arduino.h>
#include <unity.h>
#include <Web3.h>

void test_something() {
    // Your test code
    TEST_ASSERT_TRUE(true);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_something);
    UNITY_END();
}

void loop() {}
```

### Network Tests

For tests requiring Wi-Fi/RPC, prefix the directory with `test_network_*` and wrap tests in `#ifdef ENABLE_NETWORK_TESTS`:

```cpp
#ifdef ENABLE_NETWORK_TESTS

void test_network_feature() {
    // Your network test
}

#endif
```

## Test Vectors

Test vectors for cryptographic operations are verified against trusted reference implementations:

- **Keccak-256**: pycryptodome `Crypto.Hash.keccak`
- **Key derivation**: eth-keys library `PrivateKey.public_key.to_bytes()`
- **Address derivation**: eth-utils `to_checksum_address()`
- **Signing**: eth-account library personal_sign
- **RLP encoding**: web3.py rlp library
- **Transaction encoding**: ethers.js Transaction class

## Which Tests Need Funds?

**Read-only tests** (no funds needed):
- All offline tests (`test_crypto`, `test_util`, `test_transaction`)
- All tests in `test_network_rpc/`:
  - Wi-Fi connection test
  - Chain ID, block number queries
  - Balance queries (reads any address)
  - Transaction count queries
  - ERC20 token metadata (name, symbol, decimals, balances)

**Transaction tests** (require funded testnet account):
- Tests in `test_network_tx/` send REAL transactions on Sepolia testnet:
  - `test_transaction_signing_only` - No funds needed (dry-run)
  - `test_send_eth_transaction` - Sends ~0.001 ETH + gas (~0.002 ETH total)
  - `test_send_erc20_transaction` - Sends 0.1 tokens + gas (~0.002 ETH for gas)
- Set `TEST_PRIVATE_KEY` and `TEST_TO_ADDRESS` in `test_config.ini` to enable
- **SECURITY WARNING**: 
  - Use Sepolia testnet ONLY - never mainnet!
  - Generate a dedicated test key - never use keys with real funds
  - Get testnet ETH from https://sepoliafaucet.com/
- Tests will skip (TEST_IGNORE) if private key is not configured
- Chain ID verification prevents accidental mainnet sends

## CI

The GitHub Actions workflow compiles all tests to ensure they build correctly. Network tests are compiled but not executed in CI (no credentials or hardware available).
