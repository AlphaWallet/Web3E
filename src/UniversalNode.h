/**
 * Device client for universal-node signed read-only JSON-RPC.
 *
 * Routes eth_getBalance / eth_call (and other allowlisted reads) through
 * https://uninode.percolate.one with EIP-191 request signing (KeyID) and
 * response verification against a pinned node signer address.
 *
 * Requires NTP-synced device time (default window ±60s).
 */

#ifndef WEB3E_UNIVERSAL_NODE_H
#define WEB3E_UNIVERSAL_NODE_H

#include "UniversalNodeProtocol.h"
#include "Web3.h"
#include <string>

class KeyID;

#ifndef UNIVERSAL_NODE_HOST
#define UNIVERSAL_NODE_HOST "uninode.percolate.one"
#endif

#ifndef UNIVERSAL_NODE_PATH
#define UNIVERSAL_NODE_PATH "/rpc"
#endif

class UniversalNode {
public:
    /**
     * @param web3  Existing Web3 instance (provides chainId; used for Crypto context).
     * @param keyId Device KeyID used to personal_sign requests. EEPROM layout unchanged.
     */
    UniversalNode(Web3* web3, KeyID* keyId);
    ~UniversalNode();

    void setHost(const char* host, const char* path = UNIVERSAL_NODE_PATH, unsigned short port = 443);
    void setNodeSigner(const char* addressLowerOrChecksum);
    void setTimestampWindowSeconds(int seconds);

    const char* nodeSigner() const { return nodeSignerAddress.c_str(); }
    bool isTimeSynced() const { return timeSynced; }

    /**
     * Sync clock via NTP. Returns false if time could not be established.
     * Must succeed before call() / Web3 routed reads.
     */
    bool syncTime(const char* ntpServer1 = "pool.ntp.org",
                  const char* ntpServer2 = "time.nist.gov",
                  uint32_t timeoutMs = 15000);

    /** Current unix seconds (0 if not synced). */
    long long nowUnix() const;

    /**
     * Signed RPC read. paramsCanonicalJson must be canonical (use Protocol helpers).
     * On success returns a JSON object {"result":...} compatible with Web3 parsers.
     * On failure returns empty string (see lastError()).
     */
    std::string call(const std::string& method, const std::string& paramsCanonicalJson);

    uint256_t ethGetBalance(const std::string* address);
    std::string ethViewCall(const std::string* data, const char* to);
    int ethBlockNumber();
    long long ethChainId();

    /**
     * Fetch a signed best-RPC URL pair via uninode_getRpcPair (protocol v1).
     *
     * Wire (must match universal-node):
     * - Request params `[chainId]` or
     *   `[chainId,{"blocked":[url...],"working":[url...]}]` (lists optional, ≤8 each;
     *   empty lists omitted; object keys sorted). params[0] == envelope chainId.
     * - Success: signed v1 response with
     *   result `{chainId,timestamp,rpcs[]}`, blockNumber `"0"`, signature over
     *   `result:<keccak256(canonicalJson(result))>` (sorted keys, no whitespace).
     * - Errors: unsigned HTTP `{error:{code:"RPC_VALIDATION",message}}` — returns false.
     *
     * With blocked set, server returns [working node, next ranked below both].
     * With only working (refresh), may return the same pair if still healthy.
     * Blocked URLs are never returned. Pair freshness is result.timestamp.
     * On success: outCount is 1 or 2, each URL https and ≤255 bytes.
     */
    bool getRpcPair(long long chainId,
                    char outUrls[][256],
                    int& outCount,
                    uint32_t& outTimestamp,
                    long long& outChainId,
                    const char* const* blockedUrls = nullptr,
                    int blockedCount = 0,
                    const char* const* workingUrls = nullptr,
                    int workingCount = 0);

    const char* lastError() const { return lastErrorMsg.c_str(); }
    const std::string& lastBlockNumber() const { return lastVerifiedBlockNumber; }

private:
    bool ensureTimeReady();
    bool generateNonce(std::string& outNonceHex);
    bool personalSign(const std::string& message, std::string& outSigHex);
    std::string httpPost(const std::string& body);
    std::string wrapResult(const std::string& resultCanonical) const;

    Web3* web3;
    KeyID* keyId;
    std::string host;
    std::string path;
    unsigned short port;
    std::string nodeSignerAddress;
    int timestampWindowSeconds;
    bool timeSynced;
    std::string lastErrorMsg;
    std::string lastVerifiedBlockNumber;
    BYTE* clientMem;
};

#endif
