// AploEmbed main implementation
//
// Specialized for AploCoin blockchain
// Based on Web3E by James Brown (@JamesSmartCell, @AlphaWallet)
// Original Web3 Arduino by Okada, Takahiro
//

#include "Web3.h"
#include "Certificates.h"
#include "Util.h"
#include "TagReader/TagReader.h"
#include <iostream>
#include <sstream>
#include "nodes.h"

// Helper function to initialize Web3 instance
void Web3::initWeb3(const char* primaryRpc, const char* fallbackRpc) {
    mem = new BYTE[sizeof(WiFiClientSecure)];
    chainId = APLO_ID;  // Fixed to AploCoin chain ID (28282)
    primaryRpcUrl = primaryRpc;
    fallbackRpcUrl = fallbackRpc;
    selectHost();
}

// Default constructor: uses default Aplo RPC endpoints
Web3::Web3() {
    initWeb3(getAploPrimaryNode(), getAploFallbackNode());
}

// Custom RPC constructor
Web3::Web3(const char* primaryRpcUrl) {
    initWeb3(primaryRpcUrl, getAploFallbackNode());
}

// Custom RPC with fallback
Web3::Web3(const char* primaryRpcUrl, const char* fallbackRpcUrl) {
    initWeb3(primaryRpcUrl, fallbackRpcUrl);
}

// Legacy constructor for backward compatibility
Web3::Web3(long long networkId) {
    // Ignore networkId parameter, always use APLO_ID
    if (networkId != APLO_ID) {
        Serial.print("Warning: AploEmbed only supports AploCoin (chainId ");
        Serial.print(APLO_ID);
        Serial.print("), ignoring provided chainId ");
        Serial.println(networkId);
    }
    initWeb3(getAploPrimaryNode(), getAploFallbackNode());
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
    string p = "[\\\"" + *data + "\\\"]";
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

uint256_t Web3::EthGetBalance(const string* address) {
    string m = "eth_getBalance";
    string p = "[\\\"" + *address + "\\\",\\\"latest\\\"]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getUint256(&output);
}

string Web3::EthViewCall(const string* data, const char* to)
{
    string m = "eth_call";
    string p = "[{\\\"data\\\":\\\"";;
    p += data->c_str();
    p += "\\\",\\\"to\\\":\\\"";
    p += to;
    p += "\\\"}, \\\"latest\\\"]";
    string input = generateJson(&m, &p);
    return exec(&input);
}

int Web3::EthGetTransactionCount(const string* address) {
    string m = "eth_getTransactionCount";
    string p = "[\\\"" + *address + "\\\",\\\"pending\\\"]"; //in case we need to push several transactions in a row
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getInt(&output);
}

string Web3::EthCall(const string* from, const char* to, long gas, long gasPrice,
                     const string* value, const string* data) {
    // TODO use gas, gasprice and value
    string m = "eth_call";
    string p = "[{\\\"from\\\":\\\"" + *from + "\\\",\\\"to\\\":\\\""
               + *to + "\\\",\\\"data\\\":\\\"" + *data + "\\\"}, \\\"latest\\\"]";
    string input = generateJson(&m, &p);
    return exec(&input);
}

string Web3::EthSendSignedTransaction(const string* data, const uint32_t dataLen) {
    string m = "eth_sendRawTransaction";
    string p = "[\\\"" + *data + "\\\"]";
    string input = generateJson(&m, &p);
#if 0
    LOG(input);
#endif
    return exec(&input);
}

// -------------------------------
// Private

string Web3::generateJson(const string* method, const string* params) {
    return "{\\\"jsonrpc\\\":\\\"2.0\\\",\\\"method\\\":\\\"" + *method + "\\\",\\\"params\\\":" + *params + ",\\\"id\\\":0}";
}

string Web3::exec(const string* data) {
    string result;

    client = new (mem) WiFiClientSecure();
    setupCert();

    int connected = client->connect(host, port);
    if (!connected) {
        Serial.print("Unable to connect to Host: ");
        Serial.println(host);
        delay(100);
        //trigger a reset of the device
        ESP.restart();
        return "";
    }

    // Make a HTTP request:
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
    client->println(strContentLen.c_str());
    client->println();
    client->println(data->c_str());

    while (client->connected())
    {
        String line = client->readStringUntil('\\n');
        if (line == "\\r") {
            break;
        }
    }

    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client->available()) {
        char c = client->read();
        result += c;
    }
    client->flush();
    client->stop();

    client->~WiFiClientSecure();

    return result;
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

    vector<string> *v = Util::ConvertStringHexToABIArray(&parseVal);
    
    uint256_t length = uint256_t(v->at(1));
    uint32_t lengthIndex = length;

    string asciiHex;
    int index = 2;
    while (lengthIndex > 0)
    {
        Serial.println(index);
        asciiHex += v->at(index++);
        lengthIndex -= 32;
    }

    //convert ascii into string
    string text = Util::ConvertHexToASCII(asciiHex.substr(0, length*2).c_str(), length*2);
    delete v;

    return text;
}

/**
 * @brief Fetch TLS certificate for the node
 * 
 * TODO: Add AploCoin certificate if needed
 * 
 * @return const char* 
 */
void Web3::setupCert()
{
    const char *cert = getCertificate(chainId);
    if (cert != nullptr)
    {
        client->setCACert(cert);
    }
    else
    {
        // Use insecure connection for now (acceptable for embedded devices)
        client->setInsecure();
    }
}

void Web3::selectHost()
{
    std::string node = primaryRpcUrl;

    if (node.length() == 0)
    {
        Serial.println("Error: No RPC URL configured for AploCoin");
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
        port = 443;  // Default HTTPS port
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

// -------------------------------
// RPC Failover Support
// -------------------------------

string Web3::execWithFailover(const string* data) {
    // Try primary RPC first
    string result = exec(data);
    
    // Check if result is valid (non-empty and contains "result" field)
    if (result.length() > 0 && result.find("\"result\"") != string::npos) {
        return result;
    }
    
    // Primary failed, try fallback if configured
    if (fallbackRpcUrl != nullptr && strlen(fallbackRpcUrl) > 0) {
        Serial.println("Primary RPC failed, trying fallback...");
        
        // Temporarily switch to fallback
        const char* originalPrimary = primaryRpcUrl;
        primaryRpcUrl = fallbackRpcUrl;
        selectHost();
        
        result = exec(data);
        
        // Restore primary for next call
        primaryRpcUrl = originalPrimary;
        selectHost();
        
        if (result.length() > 0 && result.find("\"result\"") != string::npos) {
            Serial.println("Fallback RPC succeeded");
            return result;
        }
    }
    
    Serial.println("All RPC endpoints failed");
    return "";
}

// -------------------------------
// AploCoin Balance Helpers
// -------------------------------

uint256_t Web3::AploGetBalance(const string* address) {
    return EthGetBalance(address);
}

string Web3::AploGetBalanceInAplo(const string* address) {
    uint256_t balanceWei = EthGetBalance(address);
    // Convert from Gaplo (wei) to APLO (18 decimals)
    // 1 APLO = 10^18 Gaplo
    return Util::ConvertWeiToEthString(&balanceWei, 18);
}

// -------------------------------
// AploCoin Send Transaction
// -------------------------------

string Web3::AploSendTransaction(const string* from, const string* to, 
                                 const uint256_t* value, uint32_t nonce,
                                 unsigned long long gasPrice, uint32_t gasLimit) {
    // This is a simplified wrapper - in production you'd need to:
    // 1. Build and sign the transaction with private key
    // 2. RLP encode it
    // 3. Send via eth_sendRawTransaction
    // For now, return empty string as this requires Contract class integration
    Serial.println("AploSendTransaction: Use Contract class with SetPrivateKey for signed transactions");
    return "";
}

// -------------------------------
// AploCoin Staking Operations
// -------------------------------

string Web3::AploStake(const string* stakingContract, const uint256_t* amount) {
    // Call stake(uint256) function on staking contract
    // This requires Contract class integration for proper ABI encoding
    Serial.println("AploStake: Use Contract class for staking operations");
    return "";
}

string Web3::AploUnstake(const string* stakingContract) {
    // Call unstake() function on staking contract
    Serial.println("AploUnstake: Use Contract class for unstaking operations");
    return "";
}

uint256_t Web3::AploGetStake(const string* stakingContract, const string* account) {
    // Call getStake(address) view function
    // Build function selector: keccak256("getStake(address)")[:4] = 0x7a766460
    string functionData = "0x7a766460";
    
    // Pad address to 32 bytes (remove 0x prefix, pad left with zeros)
    string addr = *account;
    if (addr.substr(0, 2) == "0x") {
        addr = addr.substr(2);
    }
    while (addr.length() < 64) {
        addr = "0" + addr;
    }
    functionData += addr;
    
    string result = EthViewCall(&functionData, stakingContract->c_str());
    return getUint256(&result);
}

uint256_t Web3::AploGetStakeMultiplier(const string* stakingContract, const string* account) {
    // Call getMultiplier(address) view function
    // Build function selector: keccak256("getMultiplier(address)")[:4] = 0x8e2c0e96
    string functionData = "0x8e2c0e96";
    
    // Pad address to 32 bytes
    string addr = *account;
    if (addr.substr(0, 2) == "0x") {
        addr = addr.substr(2);
    }
    while (addr.length() < 64) {
        addr = "0" + addr;
    }
    functionData += addr;
    
    string result = EthViewCall(&functionData, stakingContract->c_str());
    return getUint256(&result);
}

// -------------------------------
// AploCoin Mining Helpers
// -------------------------------

bool Web3::AploGetMinerParams(const string* miningContract, const string* minerAddress,
                              uint256_t* lastBlock, uint256_t* currentDifficulty,
                              uint256_t* totalMined, uint256_t* prevHash) {
    // Call miner_params(address) view function
    // Returns: (uint256 last_block, uint256 current_difficulty, uint256 total_mined, uint256 prev_hash)
    // Function selector: keccak256("miner_params(address)")[:4] = 0x4c0f38c2
    string functionData = "0x4c0f38c2";
    
    // Pad address to 32 bytes
    string addr = *minerAddress;
    if (addr.substr(0, 2) == "0x") {
        addr = addr.substr(2);
    }
    while (addr.length() < 64) {
        addr = "0" + addr;
    }
    functionData += addr;
    
    string result = EthViewCall(&functionData, miningContract->c_str());
    
    if (result.length() == 0) {
        return false;
    }
    
    // Parse result - should contain 4 uint256 values (128 hex chars each = 64 bytes)
    string resultData = getResult(&result);
    if (resultData.length() < 256) {  // 4 * 64 hex chars
        return false;
    }
    
    // Extract each uint256 (64 hex chars each)
    *lastBlock = uint256_t(("0x" + resultData.substr(0, 64)).c_str());
    *currentDifficulty = uint256_t(("0x" + resultData.substr(64, 64)).c_str());
    *totalMined = uint256_t(("0x" + resultData.substr(128, 64)).c_str());
    *prevHash = uint256_t(("0x" + resultData.substr(192, 64)).c_str());
    
    return true;
}

string Web3::AploMine(const string* miningContract, const string* nonce) {
    // Call mine(bytes32) function
    // This requires Contract class integration for proper transaction signing
    Serial.println("AploMine: Use Contract class for mining operations");
    return "";
}
