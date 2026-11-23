/**
 * universal-node protocol v1 helpers (Arduino-free).
 *
 * Byte-exact message construction, canonical JSON, keccak helpers, and
 * EIP-191 response verification. See JamesSmartCell/universal-node docs/protocol.md.
 */

#ifndef WEB3E_UNIVERSAL_NODE_PROTOCOL_H
#define WEB3E_UNIVERSAL_NODE_PROTOCOL_H

#include <stdint.h>
#include <string>
#include <vector>

namespace UniversalNodeProtocol {

/** Default pinned node signer (production); override at runtime if needed. */
static const char* const DEFAULT_NODE_SIGNER =
    "0xF09b881B849B50D9A3d96b7Aed50f34b5F585A30";

static const int DEFAULT_TIMESTAMP_WINDOW_SECONDS = 60;

enum VerifyStatus {
    VERIFY_OK = 0,
    VERIFY_BAD_JSON,
    VERIFY_BAD_SIGNATURE,
    VERIFY_SIGNER_MISMATCH,
    VERIFY_NONCE_MISMATCH,
    VERIFY_CHAIN_MISMATCH,
    VERIFY_REQUEST_HASH_MISMATCH,
    VERIFY_TIMESTAMP_SKEW,
    VERIFY_MISSING_FIELD
};

/** Compact canonical JSON (sorted object keys, no insignificant whitespace). */
std::string canonicalJson(const std::string& jsonText);
std::string canonicalJsonString(const std::string& value);
std::string canonicalJsonNumber(long long value);
std::string canonicalJsonBool(bool value);
std::string canonicalJsonNull();

/** Build canonical params JSON helpers for common read methods. */
std::string paramsEthGetBalance(const std::string& address, const std::string& blockTag = "latest");
std::string paramsEthCall(const std::string& to, const std::string& data,
                          const std::string& blockTag = "latest");
std::string paramsEmpty();

/**
 * Canonical params for uninode_getRpcPair.
 * - No feedback: `[chainId]`
 * - With feedback: `[chainId,{"blocked":[...],"working":[...]}]` (omit empty arrays;
 *   object keys sorted). At most 8 URLs per list; extras ignored. Firmware only
 *   tags the current cached pair (≤2 URLs total across both lists).
 */
std::string paramsGetRpcPair(long long chainId,
                             const char* const* blockedUrls, int blockedCount,
                             const char* const* workingUrls, int workingCount);

/** Lowercase 0x-hex of keccak256(utf8 bytes). */
std::string keccakHex(const uint8_t* data, size_t length);
std::string keccakHex(const std::string& utf8);
std::string hashParams(const std::string& paramsCanonicalJson);
std::string hashResult(const std::string& resultCanonicalJson);

/** Normalise hex to 0x + lowercase; returns empty string on invalid input. */
std::string normaliseHex(const std::string& value);

/** Lowercase 0x address (20 bytes); empty on failure. */
std::string normaliseAddress(const std::string& address);

/**
 * Request signing message (LF-joined, no trailing newline).
 * paramsCanonicalJson must already be canonical (or empty → []).
 */
std::string buildRequestSigningMessage(long long chainId,
                                       const std::string& method,
                                       const std::string& paramsCanonicalJson,
                                       long long timestamp,
                                       const std::string& nonceHex);

std::string computeRequestHash(const std::string& requestSigningMessage);

/**
 * Response signing message (LF-joined, no trailing newline).
 * deviceAddress and nonce/requestHash should be normalised; blockNumber is decimal text.
 */
std::string buildResponseSigningMessage(const std::string& deviceAddressLower,
                                        const std::string& nonceHex,
                                        long long chainId,
                                        const std::string& requestHashHex,
                                        const std::string& blockNumberDecimal,
                                        long long timestamp,
                                        const std::string& resultCanonicalJson);

/** EIP-191 personal_sign digest (32 bytes) over message. */
void personalMessageHash(const std::string& message, uint8_t outHash[32]);

/** Recover signer address (0x lowercase) from EIP-191 personal_sign signature hex. */
std::string recoverPersonalSigner(const std::string& message, const std::string& signatureHex);

/**
 * Verify a signed universal-node response body against what the device sent.
 * responseJson: full HTTP body from POST /rpc.
 * expectedResultCanonical: if non-empty, must match canonical form of response result;
 *   if empty, result is taken from the response and hashed.
 */
VerifyStatus verifyResponse(const std::string& responseJson,
                            const std::string& deviceAddressLower,
                            const std::string& nonceHex,
                            long long chainId,
                            const std::string& requestHashHex,
                            const std::string& pinnedNodeSigner,
                            long long nowUnixSeconds,
                            int timestampWindowSeconds,
                            std::string* outResultCanonical = nullptr,
                            std::string* outBlockNumber = nullptr);

/** Extract a top-level JSON value's raw text for key (handles string/object/array/number/bool/null). */
bool extractTopLevelJsonValue(const std::string& json, const char* key, std::string& outValue);

/** Extract a top-level JSON string field (unescaped). */
bool extractTopLevelString(const std::string& json, const char* key, std::string& out);

/** Extract a top-level JSON number as int64 (also accepts numeric strings). */
bool extractTopLevelInt64(const std::string& json, const char* key, long long& out);

/**
 * Extract a top-level JSON string array for key into out (cleared first).
 * Returns false if missing, not an array, or any element is not a string.
 */
bool extractTopLevelStringArray(const std::string& json, const char* key,
                                std::vector<std::string>& out);

std::string toLowerHex(const uint8_t* bytes, size_t length);
std::string bytesToLowerHex0x(const uint8_t* bytes, size_t length);

} // namespace UniversalNodeProtocol

#endif
