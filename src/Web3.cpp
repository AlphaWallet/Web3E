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
#include <iostream>
#include <sstream>
#include <memory>
#include "nodes.h"

Web3::Web3(long long networkId) {
#ifdef USING_INFURA
    infura_key = INFURA_API_KEY;
#else
    infura_key = nullptr;
#endif
    mem = new BYTE[sizeof(WiFiClientSecure)];
    chainId = networkId;
    rpcResult.reserve(WEB3_RPC_BUFFER_SIZE);
    selectHost();
}

Web3::Web3(long long networkId, const char* infura_key_str) {
    mem = new BYTE[sizeof(WiFiClientSecure)];
    
#ifdef USING_INFURA
    if (strnlen(infura_key_str, 34) != 32)
    {
        Serial.println("Incorrect Infura Key spec, fallback to staging API key");
        infura_key = INFURA_API_KEY;
    }
    else
    {
        infura_key = infura_key_str;
        Serial.print("Setting Infura Key: ");
        Serial.println(infura_key);
    }
#else
    // Infura disabled - ignore the key
    infura_key = nullptr;
    if (infura_key_str) {
        Serial.println("Warning: Infura key provided but USING_INFURA not defined - using public RPCs");
    }
#endif

    chainId = networkId;
    rpcResult.reserve(WEB3_RPC_BUFFER_SIZE);
    selectHost();
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
    string m = "eth_blockNumber";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getInt(&output);
}

long long int Web3::EthChainId() {
    string m = "eth_chainId";
    string p = "[]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getLongLong(&output);
}

uint256_t Web3::EthGetBalance(const string* address) {
    string m = "eth_getBalance";
    string p = "[\"" + *address + "\",\"latest\"]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getUint256(&output);
}

string Web3::EthViewCall(const string* data, const char* to)
{
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
    client = new (mem) WiFiClientSecure();
    setupCert();

    int connected = client->connect(host, port);
    if (!connected) {
#if defined(TEST_BUILD) || defined(DEBUG)
        Serial.print("Web3: Unable to connect to ");
        Serial.println(host);
#endif
#ifdef WEB3_RESTART_ON_CONNECT_FAIL
        delay(100);
        ESP.restart();
#endif
        return "";
    }

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
            Serial.print("Web3: Response timeout from ");
            Serial.println(host);
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
                Serial.println("Web3: Timeout reading status line");
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
#if defined(TEST_BUILD) || defined(DEBUG)
                    Serial.print("Web3: Invalid status line: ");
                    Serial.println(httpLineBuf);
#endif
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
                    Serial.println("Web3: Timeout reading headers");
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

    // Check HTTP status code
    if (parser.statusCode < 200 || parser.statusCode >= 300) {
#if defined(TEST_BUILD) || defined(DEBUG)
        Serial.print("Web3: HTTP ");
        Serial.print(parser.statusCode);
        Serial.print(" from ");
        Serial.println(host);
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

    rpcResult.assign(rpcReadBuf, bodyLen);
    
#if defined(TEST_BUILD) || defined(DEBUG)
    // In debug/test builds, check for JSON-RPC error responses
    if (rpcResult.find("\"error\"") != string::npos) {
        Serial.println("\n=== RPC ERROR RESPONSE ===");
        string truncated = rpcResult.length() > 300 ? rpcResult.substr(0, 300) + "..." : rpcResult;
        Serial.println(truncated.c_str());
        Serial.println("==========================\n");
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
    std::string node = getNode(chainId);

#ifdef USING_INFURA
    if (node.find("infura.io") != std::string::npos && infura_key)
    {
        node += infura_key;
    }
#endif

    if (node.length() == 0)
    {
        Serial.print("ChainId: ");
        Serial.print(chainId);
        Serial.println("Is not yet supported, please add the RPC (and certificate if required) and submit a PR to the repo.");
        return;
    }

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

    ppos = node.find("/");
    if (ppos > 0)
    {
        host = strdup(node.substr(0, ppos).c_str());
        path = strdup(node.substr(ppos).c_str());
    }
    else
    {
        host = strdup(node.c_str());
        path = "/";
    }
}


long long Web3::getChainId() const {
    return chainId;
}