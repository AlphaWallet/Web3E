// AploEmbed main header
//
// Specialized for AploCoin blockchain
// Based on Web3E by James Brown (@JamesSmartCell, @AlphaWallet)
// Original Web3 Arduino by Okada, Takahiro
//

#ifndef APLOEMBED_WEB3_H
#define APLOEMBED_WEB3_H

typedef unsigned char BYTE;
#define ETHERS_PRIVATEKEY_LENGTH       32
#define ETHERS_PUBLICKEY_LENGTH        64
#define ETHERS_ADDRESS_LENGTH          20
#define ETHERS_KECCAK256_LENGTH        32
#define ETHERS_SIGNATURE_LENGTH        65

class Web3;
class Crypto;
class KeyID;

#include "stdint.h"
#include "uint256/uint256_t.h"
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <Contract.h>
#include <Crypto.h>
#include <KeyID.h>
#include <Util.h>
#include <string.h>
#include <string>
#include "AploContracts.h"

using namespace std;

enum ConnectionStage
{
    unconnected,
    handshake,
    have_token,
    confirmed
};

class Web3 {
public:
    // Default constructor: uses pub1.aplocoin.com as primary, pub2 as fallback
    // TLS uses WiFiClientSecure defaults unless a CA bundle/certificate or explicit insecure mode is configured.
    Web3();
    
    // Custom RPC constructor: specify primary RPC URL
    Web3(const char* primaryRpcUrl);
    
    // Custom RPC with fallback: specify both primary and fallback URLs
    Web3(const char* primaryRpcUrl, const char* fallbackRpcUrl);
    
    // Legacy constructor for backward compatibility (chainId ignored, always uses APLO_ID)
    Web3(long long chainId);
    
    // Certificate validation configuration
    // Call one of these methods BEFORE making any RPC calls when your platform requires explicit trust anchors.
    
    // Option 1: Use ESP32 CA certificate bundle (recommended for production)
    // Requires: board_build.embed_files = data/cert/x509_crt_bundle.bin in platformio.ini
    // See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_crt_bundle.html
    void setCertificateBundle(const uint8_t* bundle_start);
    
    // Option 2: Use specific CA certificate (PEM format)
    // Useful when you control the RPC endpoint and want to pin to a specific CA
    void setCertificate(const char* root_ca);
    
    // Option 3: Explicitly disable certificate validation (insecure, for testing only)
    // This is never enabled automatically.
    void setInsecure();
    
    string Web3ClientVersion();
    string Web3Sha3(const string* data);
    int NetVersion();
    bool NetListening();
    int NetPeerCount();
    double EthProtocolVersion();
    bool EthSyncing();
    bool EthMining();
    double EthHashrate();
    long long int EthGasPrice();
    void EthAccounts(char** array, int size);
    int EthBlockNumber();
    uint256_t EthGetBalance(const string* address);
    int EthGetTransactionCount(const string* address);
    string EthViewCall(const string* data, const char* to);

    string EthCall(const string* from, const char* to, long gas, long gasPrice, const string* value, const string* data);
    string EthSendSignedTransaction(const string* data, const uint32_t dataLen);

    // AploCoin-specific helpers
    // Balance helpers with unit conversion
    // Note: 1 APLO = 10^18 Gaplo (wei), similar to ETH/wei relationship
    uint256_t AploGetBalance(const string* address);  // Returns balance in Gaplo (wei)
    string AploGetBalanceInAplo(const string* address);  // Returns balance as APLO string (human-readable)
    
    // Staking operations (requires staking contract address, private key, and sender address)
    // All amounts in Gaplo (wei)
    // Returns transaction hash on success, empty string on failure
    string AploStake(const string* stakingContract, const uint256_t* amount, const char* privateKey, const string* fromAddress);
    string AploUnstake(const string* stakingContract, const char* privateKey, const string* fromAddress);
    uint256_t AploGetStake(const string* stakingContract, const string* account);
    uint256_t AploGetStakeMultiplier(const string* stakingContract, const string* account);
    
    // Mining helpers
    // Returns: last_block, current_difficulty, total_mined, prev_hash
    bool AploGetMinerParams(const string* miningContract, const string* minerAddress,
                           uint256_t* lastBlock, uint256_t* currentDifficulty,
                           uint256_t* totalMined, uint256_t* prevHash);
    // Submit mining transaction with valid nonce (32-byte hex string)
    // Returns transaction hash on success, empty string on failure
    string AploMine(const string* miningContract, const string* nonce, const char* privateKey, const string* fromAddress);

    long long int getLongLong(const string* json);
    string getString(const string* json);
    int getInt(const string* json);
    uint256_t getUint256(const string* json);
    long long int getChainId() const;
    string getResult(const string* json);

private:
    string exec(const string* data);
    string execWithFailover(const string* data);  // RPC failover wrapper
    string generateJson(const string* method, const string* params);
    void selectHost();
    void setupCert();
    void initWeb3(const char* primaryRpc, const char* fallbackRpc);
    
    long getLong(const string* json);
    double getDouble(const string* json);
    bool getBool(const string* json);

private:
    WiFiClientSecure *client;
    BYTE *mem;
    const char* host;
    const char* path;
    const char* primaryRpcUrl;
    const char* fallbackRpcUrl;
    unsigned short port;
    long long chainId;  // Fixed to APLO_ID (28282)
    
    // Certificate configuration state
    enum CertMode { CERT_AUTO, CERT_INSECURE, CERT_CA, CERT_BUNDLE };
    CertMode certMode;
    const char* caCert;
    const uint8_t* certBundle;
};

#endif //APLOEMBED_WEB3_H
