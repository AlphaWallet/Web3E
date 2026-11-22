// Web3E main header
//
// By James Brown Githubs: @JamesSmartCell @AlphaWallet 
// Twitters: @TallyDigital @AlphaWallet
//
// Based on Web3 Arduino by Okada, Takahiro.
//
//

#ifndef ARDUINO_WEB3_WEB3_H
#define ARDUINO_WEB3_WEB3_H

typedef unsigned char BYTE;
#define ETHERS_PRIVATEKEY_LENGTH       32
#define ETHERS_PUBLICKEY_LENGTH        64
#define ETHERS_ADDRESS_LENGTH          20
#define ETHERS_KECCAK256_LENGTH        32
#define ETHERS_SIGNATURE_LENGTH        65

// Infura API key (opt-in only)
// Define -DUSING_INFURA and -DINFURA_API_KEY="your-key-here" in build_flags to enable
#ifdef USING_INFURA
  #ifndef INFURA_API_KEY
    #define INFURA_API_KEY "a9d6b1764a464faaa8f0399958601361" // Staging key - REPLACE in production
  #endif
#endif

/** Reused staging buffer for eth RPC HTTP bodies (avoids per-byte string growth). */
#define WEB3_RPC_BUFFER_SIZE           4096
#define WEB3_HTTP_LINE_BUFFER_SIZE     256

/** HTTP timeout in milliseconds (wait for server response) */
#ifndef WEB3_HTTP_TIMEOUT_MS
#define WEB3_HTTP_TIMEOUT_MS           10000
#endif

class Web3;
class Crypto;
class KeyID;
class UniversalNode;
class RpcPairStore;

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

#ifndef WEB3_RPC_PAIR_MAX_AGE_SECONDS
#define WEB3_RPC_PAIR_MAX_AGE_SECONDS (7UL * 24UL * 60UL * 60UL) /* 7 days */
#endif

#ifndef WEB3_RPC_PAIR_CHECK_INTERVAL_MS
#define WEB3_RPC_PAIR_CHECK_INTERVAL_MS (24UL * 60UL * 60UL * 1000UL) /* ~24h */
#endif

enum ConnectionStage
{
    unconnected,
    handshake,
    have_token,
    confirmed
};

class Web3 {
public:
    Web3(long long chainId);
    Web3(long long chainId, const char* infura_key);
    ~Web3();
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
    long long int EthChainId();
    uint256_t EthGetBalance(const string* address);
    int EthGetTransactionCount(const string* address);
    string EthViewCall(const string* data, const char* to);
    long long int EthEstimateGas(const string* from, const char* to, const string* value, const string* data);
    string EthGetTransactionReceipt(const string* txHash);

    string EthCall(const string* from, const char* to, long gas, long gasPrice, const string* value, const string* data);
    string EthSendSignedTransaction(const string* data, const uint32_t dataLen);

    long long int getLongLong(const string* json);
    string getString(const string* json);
    int getInt(const string* json);
    uint256_t getUint256(const string* json);
    long long int getChainId() const;
    string getResult(const string* json);

    /**
     * Optionally route read-only RPC (balance, eth_call, blockNumber, chainId,
     * getTransactionReceipt) through a UniversalNode client. Pass nullptr to
     * restore public/Infura RPC. Does not take ownership.
     * Mutually exclusive with useCachedRpcPair (enabling one clears the other mode).
     */
    void setUniversalNode(UniversalNode* node);
    UniversalNode* getUniversalNode() const { return universalNode; }

    /**
     * Use a signed best-RPC URL pair from universal-node, cached in EEPROM.
     * Loads any valid cache immediately, then refreshes via uninode when empty,
     * chain differs, or older than max age. Normal Web3 RPC (reads and writes)
     * go directly to these public HTTPS nodes with per-request round-robin
     * load-sharing (alternating nodes to avoid rate limits); transport failure
     * still retries the same call once on the other node. Does not take
     * ownership of uninode.
     */
    void useCachedRpcPair(UniversalNode* uninode);

    /** Max cache age before refresh (default 7 days). */
    void setRpcPairMaxAgeSeconds(uint32_t seconds);

    /**
     * Call from setup (after WiFi) and from loop / WiFi reconnect checks.
     * Throttled to about once per 24h (millis, rollover-safe). Refreshes when
     * cache is empty, chain differs, or older than max age. On refresh failure
     * keeps the cached pair. Returns true if a usable pair is loaded.
     */
    bool maintainCachedRpcPair();

    bool usingCachedRpcPair() const { return cachedRpcEnabled; }
    int getCachedRpcCount() const { return cachedRpcCount; }
    const char* getCachedRpc(int index) const;
    uint32_t getCachedRpcTimestamp() const { return cachedRpcTimestamp; }

private:
    string exec(const string* data);
    string execOnce(const string* data);
    string generateJson(const string* method, const string* params);
    void selectHost();
    void setupCert();
    void freeHeapHostPath();
    bool parseAndApplyRpcUrl(const char* url);
    void applyCachedRpcIndex(int index);
    bool refreshRpcPairFromUninode();
    bool loadCachedRpcFromEeprom();
    bool cacheNeedsRefresh(uint32_t nowUnix) const;

    long getLong(const string* json);
    double getDouble(const string* json);
    bool getBool(const string* json);

private:
    WiFiClientSecure *client;
    BYTE *mem;
    const char* host;
    const char* path;
    const char* infura_key;
    unsigned short port;
    long long chainId;
    UniversalNode* universalNode;
    /** Filled each exec(); capacity retained across calls after first large response. */
    string rpcResult;
    char rpcReadBuf[WEB3_RPC_BUFFER_SIZE];
    char httpLineBuf[WEB3_HTTP_LINE_BUFFER_SIZE];

    // Cached RPC pair mode (direct public nodes; UniversalNode used only to fetch)
    bool cachedRpcEnabled;
    bool hostPathOnHeap;
    UniversalNode* rpcPairUninode;
    RpcPairStore* rpcPairStore;
    char cachedRpcUrls[2][256];
    char activeHostBuf[256];
    char activePathBuf[256];
    int cachedRpcCount;
    int cachedRpcIndex;
    uint32_t cachedRpcTimestamp;
    uint32_t cachedRpcChainId;
    uint32_t rpcPairMaxAgeSeconds;
    uint32_t rpcPairCheckIntervalMs;
    uint32_t lastRpcPairCheckMs;
    bool rpcPairCheckInitialized;
};

#endif //ARDUINO_WEB3_WEB3_H
