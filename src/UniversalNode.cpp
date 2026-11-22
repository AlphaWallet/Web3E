#include "UniversalNode.h"
#include "KeyID.h"
#include "Util.h"
#include "HttpParser.h"
#include "Web3Log.h"
#include "Trezor/rand.h"
#include <WiFiClientSecure.h>
#include <time.h>
#include <sstream>
#include <string.h>

using namespace std;
using namespace UniversalNodeProtocol;

UniversalNode::UniversalNode(Web3* web3In, KeyID* keyIdIn)
    : web3(web3In),
      keyId(keyIdIn),
      host(UNIVERSAL_NODE_HOST),
      path(UNIVERSAL_NODE_PATH),
      port(443),
      nodeSignerAddress(DEFAULT_NODE_SIGNER),
      timestampWindowSeconds(DEFAULT_TIMESTAMP_WINDOW_SECONDS),
      timeSynced(false),
      clientMem(nullptr) {
    // Normalise default signer to lowercase 0x form for comparisons
    string norm = normaliseAddress(nodeSignerAddress);
    if (!norm.empty()) nodeSignerAddress = norm;
    clientMem = new BYTE[sizeof(WiFiClientSecure)];
}

UniversalNode::~UniversalNode() {
    delete[] clientMem;
    clientMem = nullptr;
}

void UniversalNode::setHost(const char* hostIn, const char* pathIn, unsigned short portIn) {
    host = hostIn ? hostIn : UNIVERSAL_NODE_HOST;
    path = pathIn ? pathIn : UNIVERSAL_NODE_PATH;
    port = portIn;
}

void UniversalNode::setNodeSigner(const char* addressLowerOrChecksum) {
    string norm = normaliseAddress(addressLowerOrChecksum ? addressLowerOrChecksum : "");
    if (!norm.empty()) {
        nodeSignerAddress = norm;
    }
}

void UniversalNode::setTimestampWindowSeconds(int seconds) {
    if (seconds > 0) timestampWindowSeconds = seconds;
}

bool UniversalNode::syncTime(const char* ntpServer1, const char* ntpServer2, uint32_t timeoutMs) {
    timeSynced = false;
    configTime(0, 0, ntpServer1, ntpServer2);

    uint32_t start = millis();
    while ((millis() - start) < timeoutMs) {
        time_t now = time(nullptr);
        // Reject unset / epoch-near clocks; require roughly post-2020
        if (now > 1577836800LL) {
            timeSynced = true;
            WEB3E_PRINT("UniversalNode: NTP synced, unix=");
            WEB3E_PRINTLN(static_cast<long>(now));
            return true;
        }
        delay(200);
        yield();
    }

    lastErrorMsg = "NTP time sync failed; cannot sign universal-node requests";
    WEB3E_PRINTLN(lastErrorMsg.c_str());
    return false;
}

long long UniversalNode::nowUnix() const {
    if (!timeSynced) return 0;
    time_t now = time(nullptr);
    if (now < 1577836800LL) return 0;
    return static_cast<long long>(now);
}

bool UniversalNode::ensureTimeReady() {
    long long now = nowUnix();
    if (now == 0) {
        lastErrorMsg = "Device time not NTP-synced; call UniversalNode::syncTime() first";
        WEB3E_PRINTLN(lastErrorMsg.c_str());
        return false;
    }
    return true;
}

bool UniversalNode::generateNonce(string& outNonceHex) {
    uint8_t bytes[16];
    random_buffer(bytes, sizeof(bytes));
    outNonceHex = bytesToLowerHex0x(bytes, sizeof(bytes));
    return true;
}

bool UniversalNode::personalSign(const string& message, string& outSigHex) {
    if (!keyId || !keyId->hasRecoveredKey()) {
        lastErrorMsg = "KeyID not ready for signing";
        return false;
    }

    // KeyID::getSignature keccak's its input then signs. Pass the EIP-191
    // personal_sign preimage so the digest matches recoverPersonalSigner.
    ostringstream prefix;
    prefix << "\x19" << "Ethereum Signed Message:\n" << message.size();
    string preimage = prefix.str() + message;

    uint8_t signature[ETHERS_SIGNATURE_LENGTH];
    memset(signature, 0, sizeof(signature));
    keyId->getSignature(signature, reinterpret_cast<BYTE*>(const_cast<char*>(preimage.data())),
                        static_cast<int>(preimage.size()));

    // Ethereum personal_sign uses v = 27/28
    if (signature[64] < 27) {
        signature[64] = static_cast<uint8_t>(signature[64] + 27);
    }

    outSigHex = bytesToLowerHex0x(signature, ETHERS_SIGNATURE_LENGTH);
    return true;
}

string UniversalNode::httpPost(const string& body) {
    WiFiClientSecure* client = new (clientMem) WiFiClientSecure();
    // Response authenticity is bound by EIP-191 node signature; TLS is transport only.
    client->setInsecure();

    if (!client->connect(host.c_str(), port)) {
        lastErrorMsg = "Unable to connect to universal-node host";
        WEB3E_PRINTLN(lastErrorMsg.c_str());
        client->~WiFiClientSecure();
        return "";
    }

    stringstream lenSs;
    lenSs << body.size();

    client->print("POST ");
    client->print(path.c_str());
    client->println(" HTTP/1.1");
    client->print("Host: ");
    client->println(host.c_str());
    client->println("Content-Type: application/json");
    client->println("Connection: close");
    client->print("Content-Length: ");
    client->println(lenSs.str().c_str());
    client->println();
    client->print(body.c_str());

    unsigned long startTime = millis();
    while (client->connected() && !client->available()) {
        if (millis() - startTime > WEB3_HTTP_TIMEOUT_MS) {
            lastErrorMsg = "universal-node response timeout";
            client->stop();
            client->~WiFiClientSecure();
            return "";
        }
        delay(1);
        yield();
    }

    HttpParser parser;
    char lineBuf[WEB3_HTTP_LINE_BUFFER_SIZE];
    size_t n = 0;
    unsigned long lastByteTime = millis();

    // Status line
    while (client->connected() || client->available()) {
        if (!client->available()) {
            if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) break;
            delay(1);
            yield();
            continue;
        }
        int c = client->read();
        if (c < 0) break;
        lastByteTime = millis();
        if (c == '\n') {
            lineBuf[n] = '\0';
            parser.parseStatusLine(lineBuf);
            break;
        }
        if (c != '\r' && n + 1 < WEB3_HTTP_LINE_BUFFER_SIZE) {
            lineBuf[n++] = static_cast<char>(c);
        }
    }

    // Headers
    while (client->connected() || client->available()) {
        n = 0;
        while (client->connected() || client->available()) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) break;
                delay(1);
                yield();
                continue;
            }
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            if (c == '\n') break;
            if (c != '\r' && n + 1 < WEB3_HTTP_LINE_BUFFER_SIZE) {
                lineBuf[n++] = static_cast<char>(c);
            }
        }
        lineBuf[n] = '\0';
        if (n == 0) break;
        parser.parseHeader(lineBuf);
    }

    string responseBody;
    responseBody.reserve(1024);
    char readBuf[256];
    if (parser.isChunked) {
        size_t chunkBufLen = 0;
        char chunkBuf[WEB3_HTTP_LINE_BUFFER_SIZE];
        while (client->connected() || client->available()) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) break;
                delay(1);
                yield();
                continue;
            }
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            if (chunkBufLen < WEB3_HTTP_LINE_BUFFER_SIZE) {
                chunkBuf[chunkBufLen++] = static_cast<char>(c);
            }
            size_t consumed = 0;
            size_t written = 0;
            if (HttpParser::decodeChunk(chunkBuf, chunkBufLen, readBuf, sizeof(readBuf),
                                        consumed, written)) {
                responseBody.append(readBuf, written);
                if (written == 0) break;
                if (consumed < chunkBufLen) {
                    memmove(chunkBuf, chunkBuf + consumed, chunkBufLen - consumed);
                    chunkBufLen -= consumed;
                } else {
                    chunkBufLen = 0;
                }
            }
        }
    } else if (parser.hasContentLength) {
        while (responseBody.size() < parser.contentLength) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) break;
                delay(1);
                yield();
                continue;
            }
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            responseBody.push_back(static_cast<char>(c));
        }
    } else {
        while (client->connected() || client->available()) {
            if (!client->available()) {
                if (millis() - lastByteTime > WEB3_HTTP_TIMEOUT_MS) break;
                delay(1);
                yield();
                continue;
            }
            int c = client->read();
            if (c < 0) break;
            lastByteTime = millis();
            responseBody.push_back(static_cast<char>(c));
        }
    }

    client->stop();
    client->~WiFiClientSecure();

    if (parser.statusCode < 200 || parser.statusCode >= 300) {
        lastErrorMsg = "universal-node HTTP error";
        WEB3E_PRINT("UniversalNode: HTTP ");
        WEB3E_PRINTLN(parser.statusCode);
        return "";
    }
    return responseBody;
}

string UniversalNode::wrapResult(const string& resultCanonical) const {
    return string("{\"result\":") + resultCanonical + "}";
}

string UniversalNode::call(const string& method, const string& paramsCanonicalJson) {
    lastErrorMsg.clear();
    lastVerifiedBlockNumber.clear();

    if (!ensureTimeReady()) return "";
    if (!keyId || !keyId->hasRecoveredKey()) {
        lastErrorMsg = "KeyID required for universal-node requests";
        WEB3E_PRINTLN(lastErrorMsg.c_str());
        return "";
    }

    string params = paramsCanonicalJson.empty() ? paramsEmpty() : paramsCanonicalJson;
    // Ensure params are canonical
    string canonParams = canonicalJson(params);
    if (canonParams.empty()) {
        lastErrorMsg = "params JSON is not valid/canonicalisable";
        return "";
    }

    long long timestamp = nowUnix();
    string nonce;
    generateNonce(nonce);

    long long chainId = web3->getChainId();
    string requestMessage =
        buildRequestSigningMessage(chainId, method, canonParams, timestamp, nonce);
    string requestHash = computeRequestHash(requestMessage);

    string signature;
    if (!personalSign(requestMessage, signature)) {
        return "";
    }

    // Build POST body
    ostringstream body;
    body << "{\"chainId\":" << chainId
         << ",\"request\":{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"" << method
         << "\",\"params\":" << canonParams << "}"
         << ",\"timestamp\":" << timestamp
         << ",\"nonce\":\"" << nonce << "\""
         << ",\"signature\":\"" << signature << "\"}";

    string response = httpPost(body.str());
    if (response.empty()) {
        if (lastErrorMsg.empty()) lastErrorMsg = "empty response from universal-node";
        return "";
    }

    // Surface server error payloads clearly
    if (response.find("\"error\"") != string::npos && response.find("\"result\"") == string::npos) {
        lastErrorMsg = "universal-node returned error JSON";
        WEB3E_PRINTLN(response.c_str());
        return "";
    }

    string deviceAddr = normaliseAddress(keyId->getAddress());
    string resultCanon;
    string blockNumber;
    VerifyStatus st = verifyResponse(
        response, deviceAddr, nonce, chainId, requestHash, nodeSignerAddress,
        nowUnix(), timestampWindowSeconds, &resultCanon, &blockNumber);

    if (st != VERIFY_OK) {
        switch (st) {
            case VERIFY_BAD_JSON: lastErrorMsg = "response JSON invalid"; break;
            case VERIFY_BAD_SIGNATURE: lastErrorMsg = "response signature invalid"; break;
            case VERIFY_SIGNER_MISMATCH:
                lastErrorMsg = "response signer does not match pinned node signer";
                break;
            case VERIFY_NONCE_MISMATCH: lastErrorMsg = "response nonce mismatch"; break;
            case VERIFY_CHAIN_MISMATCH: lastErrorMsg = "response chainId mismatch"; break;
            case VERIFY_REQUEST_HASH_MISMATCH: lastErrorMsg = "response requestHash mismatch"; break;
            case VERIFY_TIMESTAMP_SKEW: lastErrorMsg = "response timestamp outside allowed window"; break;
            case VERIFY_MISSING_FIELD: lastErrorMsg = "response missing required field"; break;
            default: lastErrorMsg = "response verification failed"; break;
        }
        WEB3E_PRINT("UniversalNode verify failed: ");
        WEB3E_PRINTLN(lastErrorMsg.c_str());
        return "";
    }

    lastVerifiedBlockNumber = blockNumber;
    return wrapResult(resultCanon);
}

uint256_t UniversalNode::ethGetBalance(const string* address) {
    string params = paramsEthGetBalance(*address, "latest");
    string output = call("eth_getBalance", params);
    if (output.empty()) return uint256_t(0);
    return web3->getUint256(&output);
}

string UniversalNode::ethViewCall(const string* data, const char* to) {
    string params = paramsEthCall(to, *data, "latest");
    return call("eth_call", params);
}

int UniversalNode::ethBlockNumber() {
    string output = call("eth_blockNumber", paramsEmpty());
    if (output.empty()) return 0;
    return web3->getInt(&output);
}

long long UniversalNode::ethChainId() {
    string output = call("eth_chainId", paramsEmpty());
    if (output.empty()) return 0;
    return web3->getLongLong(&output);
}
