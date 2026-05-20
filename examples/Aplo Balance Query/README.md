# AploCoin Balance Query Example

This example demonstrates how to query AploCoin balances using the AploEmbed library.

## Features

- Query balance in **APLO** (human-readable format, 18 decimals)
- Query balance in **Gaplo** (raw blockchain units, smallest denomination)
- Automatic RPC failover (primary → fallback)
- Simple WiFi setup for ESP32

## Unit Conversion

AploCoin uses 18 decimal places:

- **1 APLO** = 1,000,000,000,000,000,000 Gaplo (10^18)
- **Gaplo** is the smallest unit (like cents to dollars)

- **APLO**: Human-readable unit
- **Gaplo**: Smallest blockchain unit

## Configuration

### 1. WiFi Credentials

Edit `main.cpp` and replace:

```cpp
const char *ssid = "<YOUR_SSID>";
const char *password = "<YOUR_WIFI_PASSWORD>";
```

### 2. AploCoin Address

Replace the example address with your own:

```cpp
#define APLO_ADDRESS "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb"
```

### 3. RPC Endpoints (Optional)

By default, the example uses:
- Primary: `pub1.aplocoin.com`
- Fallback: `pub2.aplocoin.com`

To use custom RPC endpoints, modify the Web3 initialization:

```cpp
// Custom single endpoint
web3 = new Web3("custom-rpc.aplocoin.com");

// Custom primary + fallback
web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
```

## Building and Running

### Using PlatformIO

```bash
cd "examples/Aplo Balance Query"
pio run --target upload
pio device monitor
```

### Using Arduino IDE

1. Copy the contents of `main.cpp` to a new sketch
2. Install the AploEmbed library (version 0.0.1+)
3. Select your ESP32 board
4. Set serial monitor to 115200 baud
5. Upload and monitor

## Expected Output

```
=== AploEmbed Balance Query Example ===

Connecting to WiFi: YourNetwork
........
WiFi connected!
IP address: 192.168.1.100

Web3 initialized with AploCoin RPC endpoints
Primary: pub1.aplocoin.com
Fallback: pub2.aplocoin.com

--- Querying Balance in APLO ---
Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Balance: 1234.567890123456789 APLO

--- Querying Balance in Gaplo (wei) ---
Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Balance: 1234567890123456789000 Gaplo
Converted: 1234.567890123456789 APLO
```

## API Reference

### Balance Query Methods

```cpp
// Get balance in Gaplo (wei) - returns uint256_t
uint256_t balanceGaplo = web3->AploGetBalance(&address);

// Get balance as APLO string (human-readable)
string balanceAplo = web3->AploGetBalanceInAplo(&address);
```

### Manual Unit Conversion

```cpp
// Convert Gaplo (wei) to APLO string
string aplo = Util::ConvertWeiToEthString(&gaploAmount, 18);

// Convert APLO to Gaplo (wei)
uint256_t gaplo = Util::ConvertToWei(1234.56, 18);
```

## Notes

- **No Private Keys Required**: Balance queries are read-only operations
- **RPC Failover**: If primary RPC fails, automatically retries with fallback
- **Network Requirements**: Requires active internet connection via WiFi
- **Testing**: For testing, use a local/mock RPC endpoint to avoid spamming public nodes

## Troubleshooting

### WiFi Connection Fails
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure ESP32 is within range

### Balance Query Returns Zero
- Verify the address is correct (must start with 0x)
- Check that RPC endpoints are accessible
- Ensure the address has been funded on AploCoin network

### Compilation Errors
- Ensure AploEmbed library version 0.0.1+ is installed
- Check that all required headers are available
- Verify ESP32 board support is installed

## Related Examples

- **Aplo Send Transaction**: Send APLO to another address
- **Aplo Staking**: Stake and unstake APLO tokens
- **Aplo Mining**: Query mining parameters and submit solutions

## License

This example is part of the AploEmbed library and follows the same license terms.
