# AploEmbed - Send Transaction Example

This example demonstrates how to send APLO currency to another address using the AploEmbed library on ESP32.

## ⚠️ CRITICAL SAFETY WARNINGS

**READ THIS BEFORE RUNNING THE CODE:**

1. **NEVER commit your private key to version control** - Keep it secret and secure
2. **Start with TEST amounts** - Use small values like 0.01 APLO for initial testing
3. **Verify recipient address** - Blockchain transactions are IRREVERSIBLE
4. **Check your balance** - Ensure sufficient funds for amount + gas fees
5. **Test environment first** - If a testnet is available, use it before mainnet
6. **Understand gas costs** - Transactions require gas fees in addition to the send amount

## What This Example Does

1. Connects to WiFi
2. Initializes Web3 with AploCoin RPC endpoints (pub1/pub2.aplocoin.com)
3. Queries your current APLO balance
4. Checks if you have sufficient balance (amount + gas buffer)
5. Signs and sends a transaction to the recipient address
6. Displays the transaction hash for tracking

## Hardware Requirements

- ESP32 development board
- WiFi network connection
- USB cable for programming and serial monitoring

## Software Requirements

- PlatformIO (recommended) or Arduino IDE
- AploEmbed library (this repository)

## Configuration Steps

### 1. WiFi Credentials

Edit `main.cpp` and set your local WiFi and wallet values:

```cpp
const char *ssid = "<YOUR_SSID>";
const char *password = "<YOUR_WIFI_PASSWORD>";
```

### 2. Wallet Configuration

**CRITICAL: Handle private keys securely!**

```cpp
// Your wallet address (public, safe to share)
#define MY_ADDRESS "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb"

// Recipient address (verify carefully!)
#define RECIPIENT_ADDRESS "0x007bEe82BDd9e866b2bd114780a47f2261C684E3"

// Your private key (64 hex characters, NO 0x prefix)
// NEVER commit this to version control!
const char *PRIVATE_KEY = "your_64_character_hex_private_key_here";
```

### 3. Transaction Amount

Adjust the send amount (default is 0.01 APLO for safety):

```cpp
#define SEND_AMOUNT_APLO 0.01  // Start small for testing
```

### 4. Gas Parameters (Optional)

Adjust gas settings if needed based on network conditions:

```cpp
#define GAS_PRICE 1000000000ULL    // 1 Gwei (adjust for network)
#define GAS_LIMIT 21000            // Standard transfer (usually sufficient)
```

## Building and Running

### Using PlatformIO (Recommended)

```bash
cd "examples/Aplo Send Transaction"
pio run --target upload
pio device monitor
```

### Using Arduino IDE

1. Open `main.cpp` in Arduino IDE
2. Select your ESP32 board from Tools → Board
3. Select the correct COM port from Tools → Port
4. Click Upload
5. Open Serial Monitor (115200 baud)

## Expected Output

```
=== AploEmbed Send Transaction Example ===

Connecting to WiFi: YourNetwork
.....
WiFi connected!
IP address: 192.168.1.100

Web3 initialized with AploCoin RPC endpoints
Primary: pub1.aplocoin.com
Fallback: pub2.aplocoin.com

My Address: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Current Balance: 1.234567 APLO

Attempting to send: 0.010000 APLO
To Address: 0x007bEe82BDd9e866b2bd114780a47f2261C684E3

--- Preparing Transaction ---

Amount: 0.010000 APLO
Amount in Gaplo (wei): 10000000000000000

Fetching nonce for: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
Nonce: 5

Transaction Parameters:
  From: 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb
  To: 0x007bEe82BDd9e866b2bd114780a47f2261C684E3
  Value: 0.010000 APLO
  Gas Price: 1 Gwei
  Gas Limit: 21000
  Nonce: 5

Signing and sending transaction...

--- Transaction Result ---

SUCCESS! Transaction sent.
Transaction Hash: 0xabcdef1234567890...

You can track this transaction on the AploCoin block explorer
(if available) using the transaction hash above.
```

## Troubleshooting

### "Insufficient balance" Error

**Cause:** Your wallet doesn't have enough APLO to cover the send amount plus gas fees.

**Solution:**
- Check your actual balance
- Reduce the send amount
- Ensure you have at least 0.001 APLO extra for gas

### "Transaction failed" Error

**Possible causes:**
1. **Invalid private key** - Verify it's 64 hex characters, no 0x prefix
2. **Invalid recipient address** - Must be valid address format (0x...)
3. **Network issues** - Check WiFi connection and RPC endpoint availability
4. **Insufficient gas** - Try increasing `GAS_PRICE` or `GAS_LIMIT`
5. **Nonce issues** - If you sent multiple transactions quickly, wait for previous ones to confirm

### WiFi Connection Fails

**Solution:**
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure your network allows ESP32 connections
- Try moving closer to the router

### RPC Endpoint Issues

**Note:** pub1.aplocoin.com and pub2.aplocoin.com are default configuration values. If these endpoints are not yet operational, you'll need to:

1. Use a local AploNode instance
2. Configure a custom RPC endpoint:

```cpp
// Use custom RPC endpoint
web3 = new Web3("your-local-node.local:8545");
```

## API Reference

### Key Functions Used

#### `Web3::AploGetBalance(address)`
Returns balance in Gaplo (wei units, 10^-18 APLO).

```cpp
string address = "0x...";
uint256_t balance = web3->AploGetBalance(&address);
```

#### `Util::ConvertToWei(amount, decimals)`
Converts human-readable amount to wei.

```cpp
// Convert 0.01 APLO to Gaplo (wei)
uint256_t wei = Util::ConvertToWei(0.01, 18);
```

#### `Util::ConvertWeiToEthString(wei, decimals)`
Converts wei to human-readable string.

```cpp
uint256_t wei = ...;
string aplo = Util::ConvertWeiToEthString(&wei, 18);
```

#### `Web3::EthGetTransactionCount(address)`
Gets the nonce (transaction count) for an address.

```cpp
string address = "0x...";
uint32_t nonce = web3->EthGetTransactionCount(&address);
```

#### `Contract::SendTransaction(...)`
Signs and sends a transaction.

```cpp
Contract contract(web3, "");  // Empty address for value transfers
contract.SetPrivateKey(PRIVATE_KEY);

string txHash = contract.SendTransaction(
    nonce,           // Transaction nonce
    gasPrice,        // Gas price in wei
    gasLimit,        // Gas limit
    &toAddress,      // Recipient address
    &value,          // Amount in wei
    &data            // Transaction data (empty for transfers)
);
```

## Units and Conversions

### APLO vs Gaplo

- **APLO** - Human-readable unit (like ETH)
- **Gaplo** - Smallest unit, also called "wei"
- **Conversion:** 1 APLO = 10^18 Gaplo

### Gas Units

- **Gas Price** - Cost per unit of gas, measured in Gaplo (wei)
  - 1 Gwei = 1,000,000,000 wei = 0.000000001 APLO
- **Gas Limit** - Maximum gas units the transaction can consume
  - Standard transfer: 21,000 gas units
- **Total Gas Cost** = Gas Price × Gas Used (≤ Gas Limit)

### Example Calculation

```
Send Amount: 0.01 APLO
Gas Price: 1 Gwei (1,000,000,000 wei)
Gas Limit: 21,000
Gas Used: 21,000 (for simple transfer)

Total Cost = 0.01 APLO + (1 Gwei × 21,000)
           = 0.01 APLO + 0.000021 APLO
           = 0.010021 APLO
```

## Security Best Practices

### Private Key Management

1. **Never hardcode in production** - Use secure storage (EEPROM, secure element)
2. **Never commit to git** - Add to .gitignore
3. **Use environment-specific keys** - Different keys for dev/test/prod
4. **Rotate regularly** - Change keys periodically
5. **Limit funds** - Don't store large amounts in embedded device wallets

### Transaction Safety

1. **Verify addresses** - Double-check recipient before sending
2. **Test with small amounts** - Always test with minimal values first
3. **Implement confirmation** - Add user confirmation for large amounts
4. **Monitor balance** - Check balance before and after transactions
5. **Handle errors** - Implement proper error handling and retry logic

### Network Security

1. **Use HTTPS/WSS** - Secure RPC connections when possible
2. **Validate responses** - Check RPC response integrity
3. **Implement timeouts** - Don't wait indefinitely for responses
4. **Use fallback RPCs** - Configure multiple RPC endpoints

## Advanced Usage

### Custom RPC Configuration

```cpp
// Single custom RPC
web3 = new Web3("custom-rpc.aplocoin.com");

// Primary + fallback RPC
web3 = new Web3("primary-rpc.aplocoin.com", "fallback-rpc.aplocoin.com");
```

### Dynamic Gas Price

Query current network gas price:

```cpp
long long gasPrice = web3->EthGasPrice();
Serial.print("Current gas price: ");
Serial.println(gasPrice);
```

### Transaction Confirmation

After sending, you can poll for transaction receipt:

```cpp
// Wait for transaction to be mined
// (Implementation depends on your use case)
delay(15000);  // Wait ~15 seconds for block confirmation

// Query transaction receipt (requires additional RPC methods)
// string receipt = web3->EthGetTransactionReceipt(&txHash);
```

## Related Examples

- **Aplo Balance Query** - Query APLO and Gaplo balances
- **Contract Interaction** - Call smart contract functions (coming soon)
- **Staking Operations** - Stake/unstake APLO (coming soon)

## Support

For issues, questions, or contributions:
- GitHub: [AploSDK Repository]
- Documentation: [AploEmbed Docs]

## License

This example is part of the AploEmbed library.
See the main repository LICENSE file for details.
