#include <Arduino.h>
#include <WiFi.h>
#include <Web3.h>
#include <Contract.h>
#include <Util.h>

// ============================================================================
// IMPORTANT SAFETY NOTES - READ BEFORE USE
// ============================================================================
//
// 1. NEVER commit your real private key to version control
// 2. Use TEST amounts only - start with small values (e.g., 0.01 APLO)
// 3. Verify recipient address carefully - transactions are irreversible
// 4. Test on a development/testnet environment first if available
// 5. Ensure sufficient balance for amount + gas fees
//
// ============================================================================

// WiFi credentials - replace with your network details
const char *ssid = "<YOUR_SSID>";
const char *password = "<YOUR_WIFI_PASSWORD>";

// Wallet configuration
// SECURITY: Replace with your actual addresses and private key
// WARNING: Keep your private key secret! Never share or commit it.
#define MY_ADDRESS "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb"      // Your wallet address
#define RECIPIENT_ADDRESS "0x007bEe82BDd9e866b2bd114780a47f2261C684E3"  // Recipient address

// CRITICAL: Replace with your actual 64-character hex private key (without 0x prefix)
// Demo key for documentation builds. Set a real 64-character private key from secure storage in production.
const char *PRIVATE_KEY = "0000000000000000000000000000000000000000000000000000000000000000";

// Transaction parameters
// Amount to send in APLO (will be converted to Gaplo/wei internally)
#define SEND_AMOUNT_APLO 0.01  // Start with a small test amount

// Gas parameters for AploCoin network
// Adjust these based on network conditions
#define GAS_PRICE 1000000000ULL    // 1 Gwei in wei
#define GAS_LIMIT 21000            // Standard transfer gas limit

Web3 *web3;
int wificounter = 0;

void setup_wifi();
void sendAploToAddress(double aplo, const char *destination);
double queryBalance(const char *address);

void setup() 
{
    Serial.begin(115200);
    Serial.println("\n\n=== AploEmbed Send Transaction Example ===\n");
    
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
    
    // Query current balance
    double balance = queryBalance(MY_ADDRESS);
    Serial.print("My Address: ");
    Serial.println(MY_ADDRESS);
    Serial.print("Current Balance: ");
    Serial.print(balance, 6);
    Serial.println(" APLO\n");
    
    // Calculate required balance (amount + gas buffer)
    double gasBuffer = 0.001;  // Small buffer for gas fees
    double requiredBalance = SEND_AMOUNT_APLO + gasBuffer;
    
    Serial.print("Attempting to send: ");
    Serial.print(SEND_AMOUNT_APLO, 6);
    Serial.println(" APLO");
    Serial.print("To Address: ");
    Serial.println(RECIPIENT_ADDRESS);
    Serial.println();
    
    // Safety check: ensure sufficient balance
    if (balance >= requiredBalance) 
    {
        sendAploToAddress(SEND_AMOUNT_APLO, RECIPIENT_ADDRESS);
    }
    else
    {
        Serial.println("ERROR: Insufficient balance!");
        Serial.print("Required: ");
        Serial.print(requiredBalance, 6);
        Serial.print(" APLO (");
        Serial.print(SEND_AMOUNT_APLO, 6);
        Serial.print(" + ");
        Serial.print(gasBuffer, 6);
        Serial.println(" gas buffer)");
        Serial.print("Available: ");
        Serial.print(balance, 6);
        Serial.println(" APLO");
        Serial.println("\nTransaction aborted for safety.");
    }
}

void loop() 
{
    // Transaction is sent once in setup()
    // Add periodic logic here if needed
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
 * Send APLO to a destination address
 * 
 * @param aplo Amount in APLO (human-readable units)
 * @param destination Recipient address (0x...)
 */
void sendAploToAddress(double aplo, const char *destination)
{
    Serial.println("--- Preparing Transaction ---\n");
    
    // Create contract object for transaction signing
    // Empty address "" means this is a simple value transfer, not a contract call
    Contract contract(web3, "");
    contract.SetPrivateKey(PRIVATE_KEY);
    
    // Convert APLO to Gaplo (wei) - 18 decimals
    uint256_t valueGaplo = Util::ConvertToWei(aplo, 18);
    
    Serial.print("Amount: ");
    Serial.print(aplo, 6);
    Serial.println(" APLO");
    Serial.print("Amount in Gaplo (wei): ");
    Serial.println(valueGaplo.str().c_str());
    Serial.println();
    
    // Get current nonce (transaction count) for the sender
    string myAddr = MY_ADDRESS;
    Serial.print("Fetching nonce for: ");
    Serial.println(myAddr.c_str());
    uint32_t nonce = (uint32_t)web3->EthGetTransactionCount(&myAddr);
    Serial.print("Nonce: ");
    Serial.println(nonce);
    Serial.println();
    
    // Prepare transaction parameters
    string destinationAddr = destination;
    string emptyData = "";  // No data for simple transfers
    
    Serial.println("Transaction Parameters:");
    Serial.print("  From: ");
    Serial.println(MY_ADDRESS);
    Serial.print("  To: ");
    Serial.println(destination);
    Serial.print("  Value: ");
    Serial.print(aplo, 6);
    Serial.println(" APLO");
    Serial.print("  Gas Price: ");
    Serial.print((unsigned long)(GAS_PRICE / 1000000000ULL));
    Serial.println(" Gwei");
    Serial.print("  Gas Limit: ");
    Serial.println(GAS_LIMIT);
    Serial.print("  Nonce: ");
    Serial.println(nonce);
    Serial.println();
    
    // Sign and send transaction
    Serial.println("Signing and sending transaction...");
    string txResult = contract.SendTransaction(
        nonce, 
        GAS_PRICE, 
        GAS_LIMIT, 
        &destinationAddr, 
        &valueGaplo, 
        &emptyData
    );
    
    // Parse transaction hash from result
    string txHash = web3->getString(&txResult);
    
    Serial.println("\n--- Transaction Result ---\n");
    
    if (txHash.length() > 0 && txHash != "0x") 
    {
        Serial.println("SUCCESS! Transaction sent.");
        Serial.print("Transaction Hash: ");
        Serial.println(txHash.c_str());
        Serial.println();
        Serial.println("You can track this transaction on the AploCoin block explorer");
        Serial.println("(if available) using the transaction hash above.");
    }
    else
    {
        Serial.println("ERROR: Transaction failed!");
        Serial.println("Possible reasons:");
        Serial.println("  - Insufficient balance for amount + gas");
        Serial.println("  - Invalid recipient address");
        Serial.println("  - Network connectivity issues");
        Serial.println("  - RPC endpoint unavailable");
        Serial.println();
        Serial.print("Raw result: ");
        Serial.println(txResult.c_str());
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
