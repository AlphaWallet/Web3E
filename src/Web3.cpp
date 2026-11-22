// Web3E main header
//
// By James Brown Githubs: @JamesSmartCell @AlphaWallet 
// Twitters: @TallyDigital @AlphaWallet
//
// Based on Web3 Arduino by Okada, Takahiro.
//
//

#include "Web3.h"
#include "Certificates.h"
#include "Util.h"
#include "TagReader/TagReader.h"
#include "HttpParser.h"
#include "UniversalNode.h"
#include "RpcPairStore.h"
#include "RpcPairPolicy.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <stdlib.h>
#include <time.h>
#include "nodes.h"
#include "Web3Log.h"

Web3::Web3(long long networkId) {
#ifdef USING_INFURA
    infura_key = INFURA_API_KEY;
#else
    infura_key = nullptr;
#endif
    mem = new BYTE[sizeof(WiFiClientSecure)];
    chainId = networkId;
    universalNode = nullptr;
    host = nullptr;
    path = nullptr;
    port = 443;
    cachedRpcEnabled = false;
    hostPathOnHeap = false;
    rpcPairUninode = nullptr;
    rpcPairStore = nullptr;
    cachedRpcCount = 0;
    cachedRpcIndex = 0;
    cachedRpcTimestamp = 0;
    cachedRpcChainId = 0;
    rpcPairMaxAgeSeconds = WEB3_RPC_PAIR_MAX_AGE_SECONDS;
    rpcPairCheckIntervalMs = WEB3_RPC_PAIR_CHECK_INTERVAL_MS;
    lastRpcPairCheckMs = 0;
    rpcPairCheckInitialized = false;
    blockedRpcCount = 0;
    lastRpcReplacementMs = 0;
    rpcReplacementEverRequested = false;
    memset(cachedRpcUrls, 0, sizeof(cachedRpcUrls));
    memset(activeHostBuf, 0, sizeof(activeHostBuf));
    memset(activePathBuf, 0, sizeof(activePathBuf));
    memset(blockedRpcUrls, 0, sizeof(blockedRpcUrls));
    memset(rpcNodeFailStreak, 0, sizeof(rpcNodeFailStreak));
    memset(rpcNodeSuccessStreak, 0, sizeof(rpcNodeSuccessStreak));
    rpcResult.reserve(WEB3_RPC_BUFFER_SIZE);
    selectHost();
}

Web3::Web3(long long networkId, const char* infura_key_str) {
    mem = new BYTE[sizeof(WiFiClientSecure)];
    
#ifdef USING_INFURA
    if (strnlen(infura_key_str, 34) != 32)
    {
        WEB3E_PRINTLN("Incorrect Infura Key spec, fallback to staging API key");
        infura_key = INFURA_API_KEY;
    }
    else
    {
        infura_key = infura_key_str;
        WEB3E_PRINT("Setting Infura Key: ");
        WEB3E_PRINTLN(infura_key);
    }
#else
    // Infura disabled - ignore the key
    infura_key = nullptr;
    if (infura_key_str) {
        WEB3E_PRINTLN("Warning: Infura key provided but USING_INFURA not defined - using public RPCs");
    }
#endif

    chainId = networkId;
    universalNode = nullptr;
    host = nullptr;
    path = nullptr;
    port = 443;
    cachedRpcEnabled = false;
    hostPathOnHeap = false;
    rpcPairUninode = nullptr;
    rpcPairStore = nullptr;
    cachedRpcCount = 0;
    cachedRpcIndex = 0;
    cachedRpcTimestamp = 0;
    cachedRpcChainId = 0;
    rpcPairMaxAgeSeconds = WEB3_RPC_PAIR_MAX_AGE_SECONDS;
    rpcPairCheckIntervalMs = WEB3_RPC_PAIR_CHECK_INTERVAL_MS;
    lastRpcPairCheckMs = 0;
    rpcPairCheckInitialized = false;
    blockedRpcCount = 0;
    lastRpcReplacementMs = 0;
    rpcReplacementEverRequested = false;
    memset(cachedRpcUrls, 0, sizeof(cachedRpcUrls));
    memset(activeHostBuf, 0, sizeof(activeHostBuf));
    memset(activePathBuf, 0, sizeof(activePathBuf));
    memset(blockedRpcUrls, 0, sizeof(blockedRpcUrls));
    memset(rpcNodeFailStreak, 0, sizeof(rpcNodeFailStreak));
    memset(rpcNodeSuccessStreak, 0, sizeof(rpcNodeSuccessStreak));
    rpcResult.reserve(WEB3_RPC_BUFFER_SIZE);
    selectHost();
}

Web3::~Web3() {
    freeHeapHostPath();
    delete rpcPairStore;
    rpcPairStore = nullptr;
    delete[] mem;
    mem = nullptr;
}

void Web3::setUniversalNode(UniversalNode* node) {
    // Signed proxy mode and cached direct-RPC mode are alternatives.
    if (node != nullptr) {
        cachedRpcEnabled = false;
        rpcPairUninode = nullptr;
    }
    universalNode = node;
}

void Web3::setRpcPairMaxAgeSeconds(uint32_t seconds) {
    if (seconds > 0) rpcPairMaxAgeSeconds = seconds;
}

const char* Web3::getCachedRpc(int index) const {
    if (index < 0 || index >= cachedRpcCount) return nullptr;
    return cachedRpcUrls[index];
}

void Web3::freeHeapHostPath() {
    if (hostPathOnHeap) {
        if (host) free(const_cast<char*>(host));
        if (path) free(const_cast<char*>(path));
        hostPathOnHeap = false;
    }
    host = nullptr;
    path = nullptr;
}

bool Web3::parseAndApplyRpcUrl(const char* url) {
    if (!url) return false;
    static const char kHttps[] = "https://";
    if (strncmp(url, kHttps, 8) != 0) return false;

    const char* rest = url + 8;
    const char* slash = strchr(rest, '/');
    const char* colon = nullptr;
    size_t hostLen = 0;
    unsigned short parsedPort = 443;

    // Find host[:port] before first '/'
    size_t authorityLen = slash ? static_cast<size_t>(slash - rest) : strlen(rest);
    if (authorityLen == 0 || authorityLen >= sizeof(activeHostBuf)) return false;

    for (size_t i = 0; i < authorityLen; i++) {
        if (rest[i] == ':') {
            colon = rest + i;
            break;
        }
    }

    if (colon) {
        hostLen = static_cast<size_t>(colon - rest);
        if (hostLen == 0 || hostLen >= sizeof(activeHostBuf)) return false;
        char portBuf[8];
        size_t portLen = authorityLen - hostLen - 1;
        if (portLen == 0 || portLen >= sizeof(portBuf)) return false;
        memcpy(portBuf, colon + 1, portLen);
        portBuf[portLen] = '\0';
        int p = atoi(portBuf);
        if (p <= 0 || p > 65535) return false;
        parsedPort = static_cast<unsigned short>(p);
    } else {
        hostLen = authorityLen;
    }

    freeHeapHostPath();
    memcpy(activeHostBuf, rest, hostLen);
    activeHostBuf[hostLen] = '\0';

    if (slash && slash[0] != '\0') {
        size_t pathLen = strlen(slash);
        if (pathLen >= sizeof(activePathBuf)) return false;
        memcpy(activePathBuf, slash, pathLen + 1);
    } else {
        activePathBuf[0] = '/';
        activePathBuf[1] = '\0';
    }

    host = activeHostBuf;
    path = activePathBuf;
    port = parsedPort;
    return true;
}

void Web3::applyCachedRpcIndex(int index) {
    if (index < 0 || index >= cachedRpcCount) return;
    cachedRpcIndex = index;
    if (!parseAndApplyRpcUrl(cachedRpcUrls[index])) {
        WEB3E_PRINT("Web3: failed to apply cached RPC URL index ");
        WEB3E_PRINTLN(index);
        return;
    }
    WEB3E_PRINT("Web3: using cached RPC ");
    WEB3E_PRINTLN(host);
}

bool Web3::loadCachedRpcFromEeprom() {
    if (!rpcPairStore) return false;
    RpcPairData data;
    if (!rpcPairStore->load(data)) {
        cachedRpcCount = 0;
        cachedRpcTimestamp = 0;
        cachedRpcChainId = 0;
        return false;
    }
    cachedRpcCount = data.count;
    cachedRpcTimestamp = data.timestamp;
    cachedRpcChainId = data.chainId;
    for (int i = 0; i < cachedRpcCount; i++) {
        memcpy(cachedRpcUrls[i], data.urls[i], data.urlLens[i] + 1);
    }
    applyCachedRpcIndex(0);
    return cachedRpcCount > 0;
}

bool Web3::cacheNeedsRefresh(uint32_t nowUnix) const {
    if (cachedRpcCount <= 0) return true;
    if (cachedRpcChainId != static_cast<uint32_t>(chainId)) return true;
    if (nowUnix == 0) return false; // cannot judge age without time
    if (cachedRpcTimestamp == 0) return true;
    // Rollover-safe age: unsigned subtract
    uint32_t age = nowUnix - cachedRpcTimestamp;
    // If timestamp is in the future (clock skew), treat as needing refresh
    if (cachedRpcTimestamp > nowUnix) return true;
    return age > rpcPairMaxAgeSeconds;
}

void Web3::resetCachedRpcHealth() {
    memset(rpcNodeFailStreak, 0, sizeof(rpcNodeFailStreak));
    memset(rpcNodeSuccessStreak, 0, sizeof(rpcNodeSuccessStreak));
}

void Web3::rememberBlockedUrl(const char* url) {
    if (!url || url[0] == '\0') return;
    for (int i = 0; i < blockedRpcCount; i++) {
        if (strcmp(blockedRpcUrls[i], url) == 0) return;
    }
    if (blockedRpcCount >= RPC_PAIR_FEEDBACK_MAX_URLS) return;
    strncpy(blockedRpcUrls[blockedRpcCount], url, sizeof(blockedRpcUrls[0]) - 1);
    blockedRpcUrls[blockedRpcCount][sizeof(blockedRpcUrls[0]) - 1] = '\0';
    blockedRpcCount++;
}

void Web3::noteCachedRpcConnectSuccess() {
    if (!cachedRpcEnabled || cachedRpcCount <= 0) return;
    int idx = cachedRpcIndex;
    if (idx < 0 || idx >= cachedRpcCount || idx > 1) return;
    rpcNodeFailStreak[idx] = 0;
    if (rpcNodeSuccessStreak[idx] < 1000000) {
        rpcNodeSuccessStreak[idx]++;
    }
}

void Web3::noteCachedRpcConnectFailure() {
    if (!cachedRpcEnabled || cachedRpcCount < 2) return;
    int idx = cachedRpcIndex;
    if (idx < 0 || idx >= cachedRpcCount || idx > 1) return;
    if (rpcNodeFailStreak[idx] < 1000000) {
        rpcNodeFailStreak[idx]++;
    }
    rpcNodeSuccessStreak[idx] = 0;

    int other = 1 - idx;
    if (!rpcPairShouldMarkBlocked(rpcNodeFailStreak[idx], rpcNodeSuccessStreak[other])) {
        return;
    }

    WEB3E_PRINT("Web3: marking cached RPC blocked after ");
    WEB3E_PRINT(rpcNodeFailStreak[idx]);
    WEB3E_PRINT(" consecutive connect/TLS failures: ");
    WEB3E_PRINTLN(cachedRpcUrls[idx]);
    rememberBlockedUrl(cachedRpcUrls[idx]);
    requestRpcPairReplacement(idx);
    // Require another N failures before the next replacement attempt.
    rpcNodeFailStreak[idx] = 0;
}

void Web3::logTlsConnectFailure(WiFiClientSecure* secureClient) {
    char errMsg[160];
    errMsg[0] = '\0';
    int err = 0;
    if (secureClient) {
        err = secureClient->lastError(errMsg, sizeof(errMsg));
    }

    WEB3E_PRINT("Web3: Unable to connect to ");
    WEB3E_PRINT(host ? host : "(null)");
    WEB3E_PRINT(" lastError=");
    WEB3E_PRINT(err);
    WEB3E_PRINT(" ");
    WEB3E_PRINTLN(errMsg);

    if (rpcPairLooksLikeTlsIntercept(err)) {
        WEB3E_PRINTLN("Web3: TLS intercepted by router/SNI filter (plaintext HTTP on 443)");
    }
}

bool Web3::refreshRpcPairFromUninode(bool withFeedback) {
    if (!rpcPairUninode || !rpcPairStore) return false;

    char fetched[2][256];
    memset(fetched, 0, sizeof(fetched));
    int count = 0;
    uint32_t ts = 0;
    long long resultChain = 0;

    const char* blockedPtrs[RPC_PAIR_FEEDBACK_MAX_URLS];
    const char* workingPtrs[2];
    int bCount = 0;
    int wCount = 0;
    if (withFeedback) {
        for (int i = 0; i < blockedRpcCount && i < RPC_PAIR_FEEDBACK_MAX_URLS; i++) {
            blockedPtrs[bCount++] = blockedRpcUrls[i];
        }
        for (int i = 0; i < cachedRpcCount && i < 2; i++) {
            if (cachedRpcUrls[i][0] != '\0') {
                workingPtrs[wCount++] = cachedRpcUrls[i];
            }
        }
    }

    if (!rpcPairUninode->getRpcPair(
            chainId, fetched, count, ts, resultChain,
            withFeedback && bCount > 0 ? blockedPtrs : nullptr,
            withFeedback ? bCount : 0,
            withFeedback && wCount > 0 ? workingPtrs : nullptr,
            withFeedback ? wCount : 0)) {
        WEB3E_PRINT("Web3: RPC pair refresh failed: ");
        WEB3E_PRINTLN(rpcPairUninode->lastError());
        return false;
    }

    const char* urlPtrs[2] = { fetched[0], fetched[1] };
    if (!rpcPairStore->save(ts, static_cast<uint32_t>(resultChain), urlPtrs, count)) {
        WEB3E_PRINTLN("Web3: failed to persist RPC pair to EEPROM");
        // Still use in-memory pair for this session
    }

    cachedRpcCount = count;
    cachedRpcTimestamp = ts;
    cachedRpcChainId = static_cast<uint32_t>(resultChain);
    for (int i = 0; i < count; i++) {
        strncpy(cachedRpcUrls[i], fetched[i], sizeof(cachedRpcUrls[i]) - 1);
        cachedRpcUrls[i][sizeof(cachedRpcUrls[i]) - 1] = '\0';
    }
    for (int i = count; i < 2; i++) {
        cachedRpcUrls[i][0] = '\0';
    }
    resetCachedRpcHealth();
    applyCachedRpcIndex(0);
    WEB3E_PRINT("Web3: refreshed RPC pair (");
    WEB3E_PRINT(count);
    WEB3E_PRINTLN(" URLs)");
    return true;
}

bool Web3::requestRpcPairReplacement(int blockedIndex) {
    if (!rpcPairUninode || !rpcPairStore) return false;
    if (cachedRpcCount < 2 || blockedIndex < 0 || blockedIndex >= cachedRpcCount) {
        return false;
    }

    uint32_t nowMs = millis();
    if (!rpcPairCanRequestReplacement(nowMs, lastRpcReplacementMs, rpcReplacementEverRequested)) {
        WEB3E_PRINTLN("Web3: RPC pair replacement rate-limited");
        return false;
    }

    // Stamp before the call so a failing uninode cannot be hammered every RPC.
    lastRpcReplacementMs = nowMs;
    rpcReplacementEverRequested = true;

    int workingIndex = 1 - blockedIndex;
    rememberBlockedUrl(cachedRpcUrls[blockedIndex]);

    const char* blockedPtrs[RPC_PAIR_FEEDBACK_MAX_URLS];
    int bCount = 0;
    for (int i = 0; i < blockedRpcCount && i < RPC_PAIR_FEEDBACK_MAX_URLS; i++) {
        blockedPtrs[bCount++] = blockedRpcUrls[i];
    }
    const char* workingPtrs[1] = { cachedRpcUrls[workingIndex] };

    WEB3E_PRINT("Web3: requesting RPC replacement; blocked=");
    WEB3E_PRINT(cachedRpcUrls[blockedIndex]);
    WEB3E_PRINT(" working=");
    WEB3E_PRINTLN(cachedRpcUrls[workingIndex]);

    char fetched[2][256];
    memset(fetched, 0, sizeof(fetched));
    int count = 0;
    uint32_t ts = 0;
    long long resultChain = 0;

    if (!rpcPairUninode->getRpcPair(chainId, fetched, count, ts, resultChain,
                                    blockedPtrs, bCount, workingPtrs, 1)) {
        WEB3E_PRINT("Web3: RPC pair replacement failed: ");
        WEB3E_PRINTLN(rpcPairUninode->lastError());
        // Keep current cache
        return false;
    }

    const char* urlPtrs[2] = { fetched[0], fetched[1] };
    if (!rpcPairStore->save(ts, static_cast<uint32_t>(resultChain), urlPtrs, count)) {
        WEB3E_PRINTLN("Web3: failed to persist replacement RPC pair");
    }

    cachedRpcCount = count;
    cachedRpcTimestamp = ts;
    cachedRpcChainId = static_cast<uint32_t>(resultChain);
    for (int i = 0; i < count; i++) {
        strncpy(cachedRpcUrls[i], fetched[i], sizeof(cachedRpcUrls[i]) - 1);
        cachedRpcUrls[i][sizeof(cachedRpcUrls[i]) - 1] = '\0';
    }
    for (int i = count; i < 2; i++) {
        cachedRpcUrls[i][0] = '\0';
    }
    resetCachedRpcHealth();
    applyCachedRpcIndex(0);
    WEB3E_PRINT("Web3: replaced RPC pair (");
    WEB3E_PRINT(count);
    WEB3E_PRINTLN(" URLs)");
    return true;
}

void Web3::useCachedRpcPair(UniversalNode* uninode) {
    if (!uninode) {
        WEB3E_PRINTLN("Web3: useCachedRpcPair requires UniversalNode");
        return;
    }

    // Direct public RPC mode — do not route reads through the uninode proxy.
    universalNode = nullptr;
    rpcPairUninode = uninode;
    cachedRpcEnabled = true;

    if (!rpcPairStore) {
        rpcPairStore = new RpcPairStore();
    }
    if (!rpcPairStore->begin()) {
        WEB3E_PRINTLN("Web3: RpcPairStore begin failed");
    }

    loadCachedRpcFromEeprom();

    // Attempt immediate refresh if needed (requires NTP already synced on uninode)
    maintainCachedRpcPair();
}

bool Web3::maintainCachedRpcPair() {
    if (!cachedRpcEnabled || !rpcPairUninode) {
        return cachedRpcCount > 0;
    }

    uint32_t nowMs = millis();
    const bool firstInit = !rpcPairCheckInitialized;
    bool dueCheck = firstInit;
    if (!dueCheck) {
        // Rollover-safe interval check
        dueCheck = (nowMs - lastRpcPairCheckMs) >= rpcPairCheckIntervalMs;
    }

    // Always refresh immediately when cache is unusable, even between intervals
    uint32_t nowUnix = 0;
    long long uninodeNow = rpcPairUninode->nowUnix();
    if (uninodeNow > 0) nowUnix = static_cast<uint32_t>(uninodeNow);

    bool needs = cacheNeedsRefresh(nowUnix);
    if (!dueCheck && !needs) {
        return cachedRpcCount > 0;
    }

    lastRpcPairCheckMs = nowMs;
    rpcPairCheckInitialized = true;

    // First call only arms the ~24h timer when the cache is still young.
    if (!needs && firstInit) {
        return cachedRpcCount > 0;
    }

    // Age/empty/wrong-chain OR periodic dueCheck: ask uninode. Periodic checks
    // send working(+blocked) feedback so a healthy pair can renew its timestamp.
    const bool withFeedback = (cachedRpcCount > 0) || (blockedRpcCount > 0);
    WEB3E_PRINTLN(needs
                      ? "Web3: refreshing RPC pair from universal-node"
                      : "Web3: periodic RPC pair refresh with working/blocked feedback");
    if (!refreshRpcPairFromUninode(withFeedback)) {
        // Keep using whatever was cached
        if (cachedRpcCount > 0) {
            WEB3E_PRINTLN("Web3: keeping previously cached RPC pair");
            return true;
        }
        return false;
    }
    return cachedRpcCount > 0;
}

string Web3::Web3ClientVersion() {
    string m = "web3_clientVersion";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getString(&output);
}

string Web3::Web3Sha3(const string* data) {
    string m = "web3_sha3";
    string p = "[\"" + *data + "\"]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getString(&output);
}

int Web3::NetVersion() {
    string m = "net_version";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getInt(&output);
}

bool Web3::NetListening() {
    string m = "net_listening";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getBool(&output);
}

int Web3::NetPeerCount() {
    string m = "net_peerCount";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getInt(&output);
}

double Web3::EthProtocolVersion() {
    string m = "eth_protocolVersion";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getDouble(&output);
}

bool Web3::EthSyncing() {
    string m = "eth_syncing";
    string p = "[]";
    string input = generateJson(&m, &p);
    string result = exec(&input);

    return getBool(&result);
}

bool Web3::EthMining() {
    string m = "eth_mining";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getBool(&output);
}

double Web3::EthHashrate() {
    string m = "eth_hashrate";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getDouble(&output);
}

long long int Web3::EthGasPrice() {
    string m = "eth_gasPrice";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getLongLong(&output);
}

void Web3::EthAccounts(char** array, int size) {
     // TODO
}

int Web3::EthBlockNumber() {
    if (universalNode) {
        return universalNode->ethBlockNumber();
    }
    string m = "eth_blockNumber";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getInt(&output);
}

long long int Web3::EthChainId() {
    if (universalNode) {
        return universalNode->ethChainId();
    }
    string m = "eth_chainId";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getLongLong(&output);
}

uint256_t Web3::EthGetBalance(const string* address) {
    if (universalNode) {
        return universalNode->ethGetBalance(address);
    }
    string m = "eth_getBalance";
    string p = "[\"" + *address + "\",\"latest\"]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getUint256(&output);
}

string Web3::EthViewCall(const string* data, const char* to)
{
    if (universalNode) {
        return universalNode->ethViewCall(data, to);
    }
    string m = "eth_call";
    string p = "[{\"data\":\"";// + *data;
    p += data->c_str();
    p += "\",\"to\":\"";
    p += to;
    p += "\"}, \"latest\"]";
    string input = generateJson(&m, &p);
    return exec(&input);
}

long long int Web3::EthEstimateGas(const string* from, const char* to, const string* value, const string* data)
{
    string m = "eth_estimateGas";
    string p = "[{";
    
    if (from != nullptr && from->length() > 0) {
        p += "\"from\":\"" + *from + "\",";
    }
    
    if (to != nullptr && strlen(to) > 0) {
        p += "\"to\":\"" + string(to) + "\",";
    }
    
    if (value != nullptr && value->length() > 0 && *value != "0" && *value != "0x0") {
        p += "\"value\":\"" + *value + "\",";
    }
    
    if (data != nullptr && data->length() > 0 && *data != "0x") {
        p += "\"data\":\"" + *data + "\",";
    }
    
    // Remove trailing comma if present
    if (p.back() == ',') {
        p = p.substr(0, p.length() - 1);
    }
    
    p += "}]";
    
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getLongLong(&output);
}

string Web3::EthGetTransactionReceipt(const string* txHash)
{
    if (universalNode) {
        string params = "[\"" + *txHash + "\"]";
        return universalNode->call("eth_getTransactionReceipt", params);
    }
    string m = "eth_getTransactionReceipt";
    string p = "[\"" + *txHash + "\"]";
    string input = generateJson(&m, &p);
    return exec(&input);
}

int Web3::EthGetTransactionCount(const string* address) {
    string m = "eth_getTransactionCount";
    string p = "[\"" + *address + "\",\"pending\"]"; //in case we need to push several transactions in a row
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getInt(&output);
}

string Web3::EthCall(const string* from, const char* to, long gas, long gasPrice,
                     const string* value, const string* data) {
    if (universalNode) {
        // Allowlisted eth_call via universal-node uses to+data (+ optional from).
        // Prefer the ViewCall-shaped params (sorted keys) when from is unused.
        (void)gas;
        (void)gasPrice;
        (void)value;
        if (from != nullptr && !from->empty()) {
            string params = "[{\"data\":\"" + *data + "\",\"from\":\"" + *from
                          + "\",\"to\":\"" + string(to) + "\"},\"latest\"]";
            return universalNode->call("eth_call", params);
        }
        return universalNode->ethViewCall(data, to);
    }
    // TODO use gas, gasprice and value
    string m = "eth_call";
    string p = "[{\"from\":\"" + *from + "\",\"to\":\""
               + *to + "\",\"data\":\"" + *data + "\"}, \"latest\"]";
    string input = generateJson(&m, &p);
    return exec(&input);
}

string Web3::EthSendSignedTransaction(const string* data, const uint32_t dataLen) {
    string m = "eth_sendRawTransaction";
    string p = "[\"" + *data + "\"]";
    string input = generateJson(&m, &p);
#if 0
    LOG(input);
#endif
    return exec(&input);
}

// -------------------------------
// Private

string Web3::generateJson(const string* method, const string* params) {
    return "{\"jsonrpc\":\"2.0\",\"method\":\"" + *method + "\",\"params\":" + *params + ",\"id\":0}";
}

string Web3::exec(const string* data) {
    // Opportunistic cache maintenance (throttled internally)
    if (cachedRpcEnabled) {
        maintainCachedRpcPair();
    }

    // Single URL (or cached mode off): no load-sharing / retry rotation.
    if (!cachedRpcEnabled || cachedRpcCount <= 1) {
        return execOnce(data);
    }

    // Load-share: each request starts on cachedRpcIndex, then advances for the
    // next call. Index is RAM-only (starts at 0 after boot; not written to EEPROM).
    const int startIndex = cachedRpcIndex;
    applyCachedRpcIndex(startIndex);
    string result = execOnce(data);

    // Transport failure: retry the same call once on the other node.
    // Application-level JSON-RPC errors (non-empty body) do not retry.
    if (result.empty()) {
        WEB3E_PRINTLN("Web3: RPC transport error — retrying on next cached node");
        applyCachedRpcIndex(rpcPairRetryIndex(startIndex, cachedRpcCount));
        result = execOnce(data);
    }

    // Advance primary index for the following request (round-robin load-share).
    applyCachedRpcIndex(rpcPairNextIndex(startIndex, cachedRpcCount));
    return result;
}

string Web3::execOnce(const string* data) {
    if (!host || !path) {
        WEB3E_PRINTLN("Web3: no RPC host configured");
        return "";
    }

    client = new (mem) WiFiClientSecure();
    setupCert();

    int connected = client->connect(host, port);
    if (!connected) {
        // Always log host + mbedtls lastError (not only under DEBUG) — COM15 /
        // AiProtection SNI blocks show up as INVALID_RECORD (-29184) / -80.
        logTlsConnectFailure(client);
        noteCachedRpcConnectFailure();
#ifdef WEB3_RESTART_ON_CONNECT_FAIL
        delay(100);
        ESP.restart();
#endif
        client->~WiFiClientSecure();
        return "";
    }
    noteCachedRpcConnectSuccess();

    // Make HTTP request with Connection: close
    int l = data->size();
    stringstream ss;
    ss << l;
    string lstr = ss.str();

    string strPost = "POST " + string(path) + " HTTP/1.1";
    string strHost = "Host: " + string(host);
    string strContentLen = "Content-Length: " + lstr;

    client->println(strPost.c_str());
    client->println(strHost.c_str());
    client->println("Content-Type: application/json");
    client->println("Connection: close");
    client->println(strContentLen.c_str());
    client->println();
    client->println(data->c_str());

    // Wait for first response bytes with timeout
    unsigned long startTime = millis();
    while (client->connected() && !client->available()) {
        if (millis() - startTime > WEB3_HTTP_TIMEOUT_MS) {
#if defined(TEST_BUILD) || defined(DEBUG)
            WEB3E_PRINT("Web3: Response timeout from ");
            WEB3E_PRINTLN(host);
#endif
            client->stop();
            client->~WiFiClientSecure();
            return "";
        }
        delay(1);
        yield();
    }

    // Parse HTTP status line
    HttpParser parser;
    bool statusParsed = false;
    size_t n = 0;
    unsigned long lastByteTime = millis();
    
    while (client->connected() || client->available()) {
        if (!client->available()) {
            if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) {
#if defined(TEST_BUILD) || defined(DEBUG)
                WEB3E_PRINTLN("Web3: Timeout reading status line");
#endif
                client->stop();
                client->~WiFiClientSecure();
                return "";
            }
            delay(1);
            yield();
            continue;
        }
        
        int c = client->read();
        if (c < 0) break;
        lastByteTime = millis();
        
        if (c == '\n') {
            httpLineBuf[n] = '\0';
            if (!statusParsed) {
                statusParsed = parser.parseStatusLine(httpLineBuf);
                if (!statusParsed) {
                    WEB3E_PRINT("Web3: Invalid status line from ");
                    WEB3E_PRINT(host ? host : "(null)");
                    WEB3E_PRINT(": ");
                    WEB3E_PRINTLN(httpLineBuf);
                    if (rpcPairLooksLikePlaintextHttp(httpLineBuf, static_cast<int>(n))) {
                        WEB3E_PRINTLN("Web3: TLS intercepted by router/SNI filter (plaintext HTTP on 443)");
                    }
                    client->stop();
                    client->~WiFiClientSecure();
                    return "";
                }
            }
            break;
        }
        
        if (c != '\r' && n + 1 < WEB3_HTTP_LINE_BUFFER_SIZE) {
            httpLineBuf[n++] = (char)c;
        }
    }

    // Parse headers until blank line
    while (client->connected() || client->available()) {
        n = 0;
        
        while (client->connected() || client->available()) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) {
#if defined(TEST_BUILD) || defined(DEBUG)
                    WEB3E_PRINTLN("Web3: Timeout reading headers");
#endif
                    client->stop();
                    client->~WiFiClientSecure();
                    return "";
                }
                delay(1);
                yield();
                continue;
            }
            
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            
            if (c == '\n') {
                break;
            }
            
            if (c != '\r' && n + 1 < WEB3_HTTP_LINE_BUFFER_SIZE) {
                httpLineBuf[n++] = (char)c;
            }
        }
        
        httpLineBuf[n] = '\0';
        
        // Blank line marks end of headers
        if (n == 0) {
            break;
        }
        
        parser.parseHeader(httpLineBuf);
    }

    // Check HTTP status code — treat as transport error for rotation
    if (parser.statusCode < 200 || parser.statusCode >= 300) {
#if defined(TEST_BUILD) || defined(DEBUG)
        WEB3E_PRINT("Web3: HTTP ");
        WEB3E_PRINT(parser.statusCode);
        WEB3E_PRINT(" from ");
        WEB3E_PRINTLN(host);
#endif
        client->stop();
        client->~WiFiClientSecure();
        return "";
    }

    // Read body (reuse member buffer, avoid per-byte std::string growth)
    size_t bodyLen = 0;
    
    if (parser.isChunked) {
        // Decode chunked transfer encoding
        size_t chunkBufLen = 0;
        
        while ((client->connected() || client->available()) && bodyLen < WEB3_RPC_BUFFER_SIZE) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) {
                    break;
                }
                delay(1);
                yield();
                continue;
            }
            
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            
            if (chunkBufLen < WEB3_HTTP_LINE_BUFFER_SIZE) {
                httpLineBuf[chunkBufLen++] = (char)c;
            }
            
            // Try to decode chunk
            size_t consumed = 0;
            size_t written = 0;
            if (HttpParser::decodeChunk(httpLineBuf, chunkBufLen,
                                       rpcReadBuf + bodyLen, WEB3_RPC_BUFFER_SIZE - bodyLen,
                                       consumed, written)) {
                bodyLen += written;
                
                // Last chunk (size 0) ends the body
                if (written == 0) {
                    break;
                }
                
                // Remove consumed bytes from chunk buffer
                if (consumed < chunkBufLen) {
                    memmove(httpLineBuf, httpLineBuf + consumed, chunkBufLen - consumed);
                    chunkBufLen -= consumed;
                } else {
                    chunkBufLen = 0;
                }
            }
        }
    } else if (parser.hasContentLength) {
        // Read exactly Content-Length bytes
        while (bodyLen < parser.contentLength && bodyLen < WEB3_RPC_BUFFER_SIZE) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) {
                    break;
                }
                delay(1);
                yield();
                continue;
            }
            
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            
            rpcReadBuf[bodyLen++] = (char)c;
        }
    } else {
        // No Content-Length, no chunked: read until connection closes or buffer full
        while ((client->connected() || client->available()) && bodyLen < WEB3_RPC_BUFFER_SIZE) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) {
                    break;
                }
                delay(1);
                yield();
                continue;
            }
            
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            
            rpcReadBuf[bodyLen++] = (char)c;
        }
    }

    client->flush();
    client->stop();
    client->~WiFiClientSecure();

    // Empty body after successful HTTP is a transport/JSON-RPC framing failure
    if (bodyLen == 0) {
        return "";
    }

    rpcResult.assign(rpcReadBuf, bodyLen);
    
#if defined(TEST_BUILD) || defined(DEBUG)
    // In debug/test builds, check for JSON-RPC error responses
    if (rpcResult.find("\"error\"") != string::npos) {
        WEB3E_PRINTLN("\n=== RPC ERROR RESPONSE ===");
        string truncated = rpcResult.length() > 300 ? rpcResult.substr(0, 300) + "..." : rpcResult;
        WEB3E_PRINTLN(truncated.c_str());
        WEB3E_PRINTLN("==========================\n");
    }
#endif
    
    return string(rpcResult);
}

int Web3::getInt(const string* json) {
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    return strtol(parseVal.c_str(), nullptr, 16);
}

long Web3::getLong(const string* json) {
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    return strtol(parseVal.c_str(), nullptr, 16);
}

long long int Web3::getLongLong(const string* json) {
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    return strtoll(parseVal.c_str(), nullptr, 16);
}

uint256_t Web3::getUint256(const string* json) {
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    return uint256_t(parseVal.c_str());
}

double Web3::getDouble(const string* json) {
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    return strtof(parseVal.c_str(), nullptr);
}

bool Web3::getBool(const string* json) {
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    long v = strtol(parseVal.c_str(), nullptr, 16);
    return v > 0;
}

string Web3::getResult(const string* json) {
    TagReader reader;
    string res = reader.getTag(json, "result");
    if (res.length() == 0)
    {
        return string("");
    }

    if (res.at(0) == 'x') res = res.substr(1);
    else if (res.at(1) == 'x') res = res.substr(2);
    return res;
}

//Currently only works for string return eg: function name() returns (string)
string Web3::getString(const string *json)
{
    TagReader reader;
    string parseVal = reader.getTag(json, "result");
    if (parseVal.length() == 0)
    {
        return string("");
    }

    std::unique_ptr<vector<string>> v(Util::ConvertStringHexToABIArray(&parseVal));
    
    // Validate we have at least offset + length words (minimum 2 words)
    if (!v || v->size() < 2)
    {
        return string("");
    }

    // Read offset word to locate the length word
    // For standalone dynamic returns, offset is typically 0x20 (32 bytes = word 1)
    uint256_t offsetWord(v->at(0));
    uint32_t offsetBytes = (uint32_t)offsetWord;
    size_t offsetWords = offsetBytes / 32;
    
    // Validate offset is within bounds
    if (offsetWords >= v->size())
    {
        return string("");
    }
    
    // Read length at the offset position
    uint256_t lengthWord(v->at(offsetWords));
    uint32_t length = (uint32_t)lengthWord;
    
    // Calculate number of 32-byte words needed for the string data
    // Each word holds up to 32 bytes, so words = ceil(length / 32)
    size_t words = (length + 31) / 32;
    
    // Validate we have enough data: offset word + length word + data words
    if (v->size() < offsetWords + 1 + words)
    {
        return string("");
    }
    
    // Extract hex characters from the data words
    string asciiHex;
    size_t dataStartIndex = offsetWords + 1;
    for (size_t i = 0; i < words; i++)
    {
        asciiHex += v->at(dataStartIndex + i);
    }
    
    // Convert only the actual string bytes (not the padding)
    uint32_t hexChars = length * 2;
    if (hexChars > asciiHex.length())
    {
        return string("");
    }
    
    string text = Util::ConvertHexToASCII(asciiHex.substr(0, hexChars).c_str(), hexChars);
    return text;
}

/**
 * @brief Fetch TLS certificate for the node
 * 
 * TODO: Add remaining certificates as required
 * 
 * @return const char* 
 */
void Web3::setupCert()
{
    // Cached public RPC pair path: no per-node certificates (setInsecure).
    // Existing nodes.h / Certificates.h path remains for non-cached mode.
    if (cachedRpcEnabled) {
        client->setInsecure();
        return;
    }

    const char *cert = getCertificate(chainId);
    if (cert != NULL)
    {
        client->setCACert(cert);
    }
    else
    {
        client->setInsecure();
    }
}

void Web3::selectHost()
{
    const char* nodeStr = getNode(chainId);
    
    // Null-check for safety (getNode should return "" not NULL, but be defensive)
    if (nodeStr == NULL || strlen(nodeStr) == 0)
    {
        WEB3E_PRINT("ChainId: ");
        WEB3E_PRINT(chainId);
        WEB3E_PRINTLN(" is not yet supported, please add the RPC (and certificate if required) and submit a PR to the repo.");
        return;
    }
    
    std::string node = nodeStr;

#ifdef USING_INFURA
    if (node.find("infura.io") != std::string::npos && infura_key)
    {
        node += infura_key;
    }
#endif

    int ppos = node.find(":");
    if (ppos > 0)
    {
        port = stoi(node.substr(ppos+1));
        node = node.substr(0, ppos);
    }
    else
    {
        port = 443;
    }

    freeHeapHostPath();

    ppos = node.find("/");
    if (ppos > 0)
    {
        host = strdup(node.substr(0, ppos).c_str());
        path = strdup(node.substr(ppos).c_str());
    }
    else
    {
        host = strdup(node.c_str());
        path = strdup("/");
    }
    hostPathOnHeap = true;
}


long long Web3::getChainId() const {
    return chainId;
}