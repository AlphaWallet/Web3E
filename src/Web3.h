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
#include "chainIds.h"

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
    Web3();
    
    // Custom RPC constructor: specify primary RPC URL
    Web3(const char* primaryRpcUrl);
    
    // Custom RPC with fallback: specify both primary and fallback URLs
    Web3(const char* primaryRpcUrl, const char* fallbackRpcUrl);
    
    // Legacy constructor for backward compatibility (chainId ignored, always uses APLO_ID)
    Web3(long long chainId);
    
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
    
    // Send APLO currency with RPC failover
    // value: amount in Gaplo (wei)
    // Returns transaction hash on success, empty string on failure
    string AploSendTransaction(const string* from, const string* to, 
                               const uint256_t* value, uint32_t nonce,
                               unsigned long long gasPrice, uint32_t gasLimit);
    
    // Staking operations (requires staking contract address)
    // All amounts in Gaplo (wei)
    string AploStake(const string* stakingContract, const uint256_t* amount);
    string AploUnstake(const string* stakingContract);
    uint256_t AploGetStake(const string* stakingContract, const string* account);
    uint256_t AploGetStakeMultiplier(const string* stakingContract, const string* account);
    
    // Mining helpers
    // Returns: last_block, current_difficulty, total_mined, prev_hash
    bool AploGetMinerParams(const string* miningContract, const string* minerAddress,
                           uint256_t* lastBlock, uint256_t* currentDifficulty,
                           uint256_t* totalMined, uint256_t* prevHash);
    string AploMine(const string* miningContract, const string* nonce);

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
};

#endif //APLOEMBED_WEB3_H
