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
#include "cJSON/cJSON.h"
#include <iostream>
#include <sstream>
#include "nodes.h"

Web3::Web3(long long networkId) {
    infura_key = INFURA_API_KEY;
    mem = new BYTE[sizeof(WiFiClientSecure)];
    chainId = networkId;
    selectHost();
}

Web3::Web3(long long networkId, const char* infura_key_str) {
    mem = new BYTE[sizeof(WiFiClientSecure)];
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

    chainId = networkId;
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

    cJSON *root, *value;
    root = cJSON_Parse(result.c_str());
    value = cJSON_GetObjectItem(root, "result");
    bool ret;
    if (cJSON_IsBool(value)) {
        ret = false;
    } else{
        ret = true;
    }
    cJSON_free(root);
    return ret;
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
        String line = client->readStringUntil('\n');
        if (line == "\r") {
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
    int ret = -1;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsString(value)) {
        ret = strtol(value->valuestring, nullptr, 16);
    }
    cJSON_free(root);
    return ret;
}

long Web3::getLong(const string* json) {
    long ret = -1;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsString(value)) {
        ret = strtol(value->valuestring, nullptr, 16);
    }
    cJSON_free(root);
    return ret;
}

long long int Web3::getLongLong(const string* json) {
    long long int ret = -1;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsString(value)) {
        ret = strtoll(value->valuestring, nullptr, 16);
    }
    cJSON_free(root);
    return ret;
}

uint256_t Web3::getUint256(const string* json) {
    uint256_t ret = 0;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsString(value)) {
        ret = uint256_t(value->valuestring);
    }
    cJSON_free(root);
    return ret;
}

double Web3::getDouble(const string* json) {
    double ret = -1;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsString(value)) {
        ret = strtof(value->valuestring, nullptr);
    }
    cJSON_free(root);
    return ret;
}

bool Web3::getBool(const string* json) {
    bool ret = false;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsBool(value)) {
        ret = (bool)value->valueint;
    }
    cJSON_free(root);
    return ret;
}

string Web3::getString(const string *json)
{
    cJSON *root, *value;
    if (json->find("result") >= 0)
    {
        root = cJSON_Parse(json->c_str());
        value = cJSON_GetObjectItem(root, "result");
        if (value != NULL && cJSON_IsString(value))
        {
            cJSON_free(root);
            return string(value->valuestring);
        }
        cJSON_free(root);
    }
    return string("");
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

    if (node.find("infura.io") != std::string::npos)
    {
        node += infura_key;
    }

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