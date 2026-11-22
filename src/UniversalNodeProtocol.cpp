#include "UniversalNodeProtocol.h"
#include "Trezor/sha3.h"
#include "Trezor/ecdsa.h"
#include "Trezor/secp256k1.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace UniversalNodeProtocol {

namespace {

static bool isHexChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static char toLowerHexNibble(uint8_t v) {
    return "0123456789abcdef"[v & 0xf];
}

static void skipWs(const std::string& s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) {
        ++i;
    }
}

static std::string escapeJsonString(const std::string& value) {
    std::string out;
    out.push_back('"');
    for (size_t i = 0; i < value.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    out += "\\u00";
                    out.push_back(toLowerHexNibble(c >> 4));
                    out.push_back(toLowerHexNibble(c));
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
    out.push_back('"');
    return out;
}

// Forward
static bool parseValue(const std::string& s, size_t& i, std::string& outCanon);

static bool parseStringRaw(const std::string& s, size_t& i, std::string& decoded) {
    if (i >= s.size() || s[i] != '"') return false;
    ++i;
    decoded.clear();
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') return true;
        if (c == '\\') {
            if (i >= s.size()) return false;
            char e = s[i++];
            switch (e) {
                case '"': decoded.push_back('"'); break;
                case '\\': decoded.push_back('\\'); break;
                case '/': decoded.push_back('/'); break;
                case 'b': decoded.push_back('\b'); break;
                case 'f': decoded.push_back('\f'); break;
                case 'n': decoded.push_back('\n'); break;
                case 'r': decoded.push_back('\r'); break;
                case 't': decoded.push_back('\t'); break;
                case 'u': {
                    if (i + 4 > s.size()) return false;
                    unsigned int code = 0;
                    for (int k = 0; k < 4; ++k) {
                        char h = s[i++];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code |= (h - '0');
                        else if (h >= 'a' && h <= 'f') code |= (h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') code |= (h - 'A' + 10);
                        else return false;
                    }
                    if (code < 0x80) {
                        decoded.push_back(static_cast<char>(code));
                    } else if (code < 0x800) {
                        decoded.push_back(static_cast<char>(0xc0 | (code >> 6)));
                        decoded.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                    } else {
                        decoded.push_back(static_cast<char>(0xe0 | (code >> 12)));
                        decoded.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                        decoded.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                    }
                    break;
                }
                default:
                    return false;
            }
        } else {
            decoded.push_back(c);
        }
    }
    return false;
}

static bool parseNumberCanon(const std::string& s, size_t& i, std::string& outCanon) {
    size_t start = i;
    if (i < s.size() && s[i] == '-') ++i;
    if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) return false;
    if (s[i] == '0') {
        ++i;
    } else {
        while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) ++i;
    }
    if (i < s.size() && s[i] == '.') {
        ++i;
        if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) return false;
        while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) ++i;
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        ++i;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
        if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) return false;
        while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) ++i;
    }
    // Re-emit via JSON.stringify-equivalent for integers; keep literal for floats/exp.
    std::string literal = s.substr(start, i - start);
    // For simple integers, ensure no leading zeros issues already handled.
    outCanon = literal;
    return true;
}

static bool parseArray(const std::string& s, size_t& i, std::string& outCanon) {
    if (i >= s.size() || s[i] != '[') return false;
    ++i;
    skipWs(s, i);
    std::string parts;
    bool first = true;
    while (i < s.size() && s[i] != ']') {
        if (!first) {
            if (s[i] != ',') return false;
            ++i;
            skipWs(s, i);
        }
        std::string elem;
        if (!parseValue(s, i, elem)) return false;
        if (!first) parts.push_back(',');
        parts += elem;
        first = false;
        skipWs(s, i);
    }
    if (i >= s.size() || s[i] != ']') return false;
    ++i;
    outCanon = "[" + parts + "]";
    return true;
}

static bool parseObject(const std::string& s, size_t& i, std::string& outCanon) {
    if (i >= s.size() || s[i] != '{') return false;
    ++i;
    skipWs(s, i);
    std::vector<std::pair<std::string, std::string> > entries;
    while (i < s.size() && s[i] != '}') {
        if (!entries.empty()) {
            if (s[i] != ',') return false;
            ++i;
            skipWs(s, i);
        }
        std::string key;
        if (!parseStringRaw(s, i, key)) return false;
        skipWs(s, i);
        if (i >= s.size() || s[i] != ':') return false;
        ++i;
        skipWs(s, i);
        std::string val;
        if (!parseValue(s, i, val)) return false;
        entries.push_back(std::make_pair(key, val));
        skipWs(s, i);
    }
    if (i >= s.size() || s[i] != '}') return false;
    ++i;
    std::sort(entries.begin(), entries.end(),
              [](const std::pair<std::string, std::string>& a,
                 const std::pair<std::string, std::string>& b) {
                  return a.first < b.first;
              });
    std::string parts;
    for (size_t n = 0; n < entries.size(); ++n) {
        if (n) parts.push_back(',');
        parts += escapeJsonString(entries[n].first);
        parts.push_back(':');
        parts += entries[n].second;
    }
    outCanon = "{" + parts + "}";
    return true;
}

static bool parseValue(const std::string& s, size_t& i, std::string& outCanon) {
    skipWs(s, i);
    if (i >= s.size()) return false;
    char c = s[i];
    if (c == '"') {
        std::string decoded;
        if (!parseStringRaw(s, i, decoded)) return false;
        outCanon = escapeJsonString(decoded);
        return true;
    }
    if (c == '{') return parseObject(s, i, outCanon);
    if (c == '[') return parseArray(s, i, outCanon);
    if (c == 't') {
        if (s.compare(i, 4, "true") != 0) return false;
        i += 4;
        outCanon = "true";
        return true;
    }
    if (c == 'f') {
        if (s.compare(i, 5, "false") != 0) return false;
        i += 5;
        outCanon = "false";
        return true;
    }
    if (c == 'n') {
        if (s.compare(i, 4, "null") != 0) return false;
        i += 4;
        outCanon = "null";
        return true;
    }
    if (c == '-' || isdigit(static_cast<unsigned char>(c))) {
        return parseNumberCanon(s, i, outCanon);
    }
    return false;
}

static bool hexToBytes(const std::string& hexIn, std::vector<uint8_t>& out) {
    std::string hex = hexIn;
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        hex = hex.substr(2);
    }
    if (hex.empty() || (hex.size() % 2) != 0) return false;
    out.resize(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        char a = hex[i * 2];
        char b = hex[i * 2 + 1];
        if (!isHexChar(a) || !isHexChar(b)) return false;
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return c - 'A' + 10;
        };
        out[i] = (nibble(a) << 4) | nibble(b);
    }
    return true;
}

static std::string addressFromPubkey(const uint8_t pubkey[64]) {
    uint8_t hash[32];
    keccak_256(pubkey, 64, hash);
    return bytesToLowerHex0x(hash + 12, 20);
}

} // namespace

std::string toLowerHex(const uint8_t* bytes, size_t length) {
    std::string out;
    out.resize(length * 2);
    for (size_t i = 0; i < length; ++i) {
        out[i * 2] = toLowerHexNibble(bytes[i] >> 4);
        out[i * 2 + 1] = toLowerHexNibble(bytes[i]);
    }
    return out;
}

std::string bytesToLowerHex0x(const uint8_t* bytes, size_t length) {
    return "0x" + toLowerHex(bytes, length);
}

std::string canonicalJson(const std::string& jsonText) {
    size_t i = 0;
    std::string out;
    if (!parseValue(jsonText, i, out)) return std::string();
    skipWs(jsonText, i);
    if (i != jsonText.size()) return std::string();
    return out;
}

std::string canonicalJsonString(const std::string& value) {
    return escapeJsonString(value);
}

std::string canonicalJsonNumber(long long value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

std::string canonicalJsonBool(bool value) {
    return value ? "true" : "false";
}

std::string canonicalJsonNull() {
    return "null";
}

std::string paramsEmpty() {
    return "[]";
}

std::string paramsEthGetBalance(const std::string& address, const std::string& blockTag) {
    // Already canonical: array of two strings
    return "[" + escapeJsonString(address) + "," + escapeJsonString(blockTag) + "]";
}

std::string paramsEthCall(const std::string& to, const std::string& data, const std::string& blockTag) {
    // Object keys sorted: data before to
    std::string obj = "{";
    obj += escapeJsonString("data") + ":" + escapeJsonString(data) + ",";
    obj += escapeJsonString("to") + ":" + escapeJsonString(to);
    obj += "}";
    return "[" + obj + "," + escapeJsonString(blockTag) + "]";
}

std::string keccakHex(const uint8_t* data, size_t length) {
    uint8_t hash[32];
    keccak_256(data, length, hash);
    return bytesToLowerHex0x(hash, 32);
}

std::string keccakHex(const std::string& utf8) {
    return keccakHex(reinterpret_cast<const uint8_t*>(utf8.data()), utf8.size());
}

std::string hashParams(const std::string& paramsCanonicalJson) {
    const std::string& p = paramsCanonicalJson.empty() ? paramsEmpty() : paramsCanonicalJson;
    return keccakHex(p);
}

std::string hashResult(const std::string& resultCanonicalJson) {
    return keccakHex(resultCanonicalJson);
}

std::string normaliseHex(const std::string& value) {
    std::string v = value;
    if (v.size() >= 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
        v = v.substr(2);
    }
    if (v.empty() || (v.size() % 2) != 0) return std::string();
    for (size_t i = 0; i < v.size(); ++i) {
        if (!isHexChar(v[i])) return std::string();
        v[i] = static_cast<char>(tolower(static_cast<unsigned char>(v[i])));
    }
    return "0x" + v;
}

std::string normaliseAddress(const std::string& address) {
    std::string h = normaliseHex(address);
    if (h.size() != 42) return std::string(); // 0x + 40
    return h;
}

std::string buildRequestSigningMessage(long long chainId,
                                       const std::string& method,
                                       const std::string& paramsCanonicalJson,
                                       long long timestamp,
                                       const std::string& nonceHex) {
    std::string nonce = normaliseHex(nonceHex);
    std::ostringstream ss;
    ss << "universal-node.request.v1\n"
       << "chainId:" << chainId << "\n"
       << "method:" << method << "\n"
       << "params:" << hashParams(paramsCanonicalJson) << "\n"
       << "timestamp:" << timestamp << "\n"
       << "nonce:" << nonce;
    return ss.str();
}

std::string computeRequestHash(const std::string& requestSigningMessage) {
    return keccakHex(requestSigningMessage);
}

std::string buildResponseSigningMessage(const std::string& deviceAddressLower,
                                        const std::string& nonceHex,
                                        long long chainId,
                                        const std::string& requestHashHex,
                                        const std::string& blockNumberDecimal,
                                        long long timestamp,
                                        const std::string& resultCanonicalJson) {
    std::string device = normaliseAddress(deviceAddressLower);
    std::string nonce = normaliseHex(nonceHex);
    std::string requestHash = normaliseHex(requestHashHex);
    std::ostringstream ss;
    ss << "universal-node.response.v1\n"
       << "device:" << device << "\n"
       << "nonce:" << nonce << "\n"
       << "chainId:" << chainId << "\n"
       << "requestHash:" << requestHash << "\n"
       << "blockNumber:" << blockNumberDecimal << "\n"
       << "timestamp:" << timestamp << "\n"
       << "result:" << hashResult(resultCanonicalJson);
    return ss.str();
}

void personalMessageHash(const std::string& message, uint8_t outHash[32]) {
    std::ostringstream prefix;
    prefix << "\x19" << "Ethereum Signed Message:\n" << message.size();
    std::string full = prefix.str() + message;
    keccak_256(reinterpret_cast<const uint8_t*>(full.data()), full.size(), outHash);
}

std::string recoverPersonalSigner(const std::string& message, const std::string& signatureHex) {
    std::vector<uint8_t> sigBytes;
    if (!hexToBytes(signatureHex, sigBytes) || sigBytes.size() != 65) {
        return std::string();
    }
    uint8_t digest[32];
    personalMessageHash(message, digest);

    uint8_t v = sigBytes[64];
    if (v > 26) v = static_cast<uint8_t>(v - 27);

    uint8_t pubkey[65];
    const ecdsa_curve* curve = &secp256k1;
    int res = ecdsa_recover_pub_from_sig(curve, pubkey, sigBytes.data(), digest, v);
    if (res != 0) return std::string();
    // pubkey[0] is 0x04; address from uncompressed X||Y
    return addressFromPubkey(pubkey + 1);
}

bool extractTopLevelJsonValue(const std::string& json, const char* key, std::string& outValue) {
    // Find "key" on the root object only (depth == 1 after the opening '{').
    const std::string needle = std::string("\"") + key + "\"";
    int depth = 0;
    bool inString = false;
    bool escape = false;
    for (size_t i = 0; i < json.size(); ++i) {
        char c = json[i];
        if (inString) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            if (depth == 1) {
                if (json.compare(i, needle.size(), needle) == 0) {
                    size_t j = i + needle.size();
                    skipWs(json, j);
                    if (j < json.size() && json[j] == ':') {
                        ++j;
                        skipWs(json, j);
                        size_t start = j;
                        size_t parsePos = j;
                        std::string canon;
                        if (!parseValue(json, parsePos, canon)) return false;
                        outValue = json.substr(start, parsePos - start);
                        while (!outValue.empty() &&
                               (outValue.back() == ' ' || outValue.back() == '\n' ||
                                outValue.back() == '\r' || outValue.back() == '\t')) {
                            outValue.pop_back();
                        }
                        return true;
                    }
                }
            }
            inString = true;
            continue;
        }
        if (c == '{' || c == '[') ++depth;
        else if (c == '}' || c == ']') {
            if (depth > 0) --depth;
        }
    }
    return false;
}

bool extractTopLevelString(const std::string& json, const char* key, std::string& out) {
    std::string raw;
    if (!extractTopLevelJsonValue(json, key, raw)) return false;
    size_t i = 0;
    skipWs(raw, i);
    std::string decoded;
    if (!parseStringRaw(raw, i, decoded)) return false;
    out = decoded;
    return true;
}

bool extractTopLevelInt64(const std::string& json, const char* key, long long& out) {
    std::string raw;
    if (!extractTopLevelJsonValue(json, key, raw)) return false;
    size_t i = 0;
    skipWs(raw, i);
    if (i < raw.size() && raw[i] == '"') {
        std::string decoded;
        if (!parseStringRaw(raw, i, decoded)) return false;
        char* end = nullptr;
        out = strtoll(decoded.c_str(), &end, 10);
        return end && *end == '\0';
    }
    char* end = nullptr;
    out = strtoll(raw.c_str(), &end, 10);
    return end && end != raw.c_str();
}

VerifyStatus verifyResponse(const std::string& responseJson,
                            const std::string& deviceAddressLower,
                            const std::string& nonceHex,
                            long long chainId,
                            const std::string& requestHashHex,
                            const std::string& pinnedNodeSigner,
                            long long nowUnixSeconds,
                            int timestampWindowSeconds,
                            std::string* outResultCanonical,
                            std::string* outBlockNumber) {
    std::string resultRaw;
    if (!extractTopLevelJsonValue(responseJson, "result", resultRaw)) {
        return VERIFY_MISSING_FIELD;
    }
    std::string resultCanon = canonicalJson(resultRaw);
    if (resultCanon.empty()) return VERIFY_BAD_JSON;

    std::string blockNumber;
    {
        std::string blockRaw;
        if (!extractTopLevelJsonValue(responseJson, "blockNumber", blockRaw)) {
            return VERIFY_MISSING_FIELD;
        }
        size_t bi = 0;
        skipWs(blockRaw, bi);
        if (bi < blockRaw.size() && blockRaw[bi] == '"') {
            if (!extractTopLevelString(responseJson, "blockNumber", blockNumber)) {
                return VERIFY_MISSING_FIELD;
            }
        } else {
            // decimal number — use canonical number text
            blockNumber = canonicalJson(blockRaw);
            if (blockNumber.empty()) return VERIFY_BAD_JSON;
        }
    }

    long long respTs = 0;
    if (!extractTopLevelInt64(responseJson, "timestamp", respTs)) {
        return VERIFY_MISSING_FIELD;
    }

    long long respChain = 0;
    if (!extractTopLevelInt64(responseJson, "chainId", respChain)) {
        return VERIFY_MISSING_FIELD;
    }
    if (respChain != chainId) return VERIFY_CHAIN_MISMATCH;

    std::string respNonce;
    if (!extractTopLevelString(responseJson, "nonce", respNonce)) {
        // nonce may not be in response body per protocol — device checks via rebuilt message
        // Protocol response JSON does NOT include nonce; nonce is in the signed message only.
        // So we use the nonce we sent.
        respNonce = nonceHex;
    } else {
        if (normaliseHex(respNonce) != normaliseHex(nonceHex)) return VERIFY_NONCE_MISMATCH;
    }

    std::string respRequestHash;
    if (!extractTopLevelString(responseJson, "requestHash", respRequestHash)) {
        return VERIFY_MISSING_FIELD;
    }
    if (normaliseHex(respRequestHash) != normaliseHex(requestHashHex)) {
        return VERIFY_REQUEST_HASH_MISMATCH;
    }

    if (timestampWindowSeconds > 0) {
        long long delta = respTs - nowUnixSeconds;
        if (delta < 0) delta = -delta;
        if (delta > timestampWindowSeconds) return VERIFY_TIMESTAMP_SKEW;
    }

    std::string signature;
    if (!extractTopLevelString(responseJson, "signature", signature)) {
        return VERIFY_MISSING_FIELD;
    }

    std::string message = buildResponseSigningMessage(
        deviceAddressLower, nonceHex, chainId, requestHashHex,
        blockNumber, respTs, resultCanon);

    std::string recovered = recoverPersonalSigner(message, signature);
    if (recovered.empty()) return VERIFY_BAD_SIGNATURE;

    std::string pinned = normaliseAddress(pinnedNodeSigner);
    if (pinned.empty() || recovered != pinned) return VERIFY_SIGNER_MISMATCH;

    // Optional: also check "signer" field if present
    std::string signerField;
    if (extractTopLevelString(responseJson, "signer", signerField)) {
        if (normaliseAddress(signerField) != pinned) return VERIFY_SIGNER_MISMATCH;
    }

    if (outResultCanonical) *outResultCanonical = resultCanon;
    if (outBlockNumber) *outBlockNumber = blockNumber;
    return VERIFY_OK;
}

} // namespace UniversalNodeProtocol
