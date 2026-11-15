# Web3E Test Suite

PlatformIO Unity test suite for Web3E library on ESP32.

## Test Structure

- **`test_crypto/`** - Cryptography tests (Keccak-256, key derivation, signing, recovery)
- **`test_util/`** - Utility function tests (hex conversion, Wei/Eth conversion, uint256, RLP)
- **`test_transaction/`** - Transaction signing and ABI encoding tests
- **`test_network_rpc/`** - Network tests requiring Wi-Fi and RPC node (optional)

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
   WIFI_SSID=YourNetworkName
   WIFI_PASS=YourWiFiPassword
   RPC_URL=https://sepolia.infura.io/v3/your-api-key
   ```

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

Test vectors for cryptographic operations are documented in each test file. They are generated using trusted reference implementations:

- **Keccak-256, signing:** ethers.js v6
- **RLP encoding:** web3.py rlp library
- **Transaction encoding:** ethers.js Transaction class

## CI

The GitHub Actions workflow compiles all tests to ensure they build correctly. Network tests are compiled but not executed in CI (no credentials available).
