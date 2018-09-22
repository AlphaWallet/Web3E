//
// Created by Okada, Takahiro on 2018/02/04.
//

#include "Web3.h"
#include <WiFiClientSecure.h>
#include "CaCert.h"
#include "Log.h"
#include "Util.h"
#include "cJSON/cJSON.h"
#include <iostream>
#include <sstream>

WiFiClientSecure client;
Log debug;
#define LOG(x) debug.println(x)

Web3::Web3(const char* _host, const char* _path) {
    client.setCACert(infura_ca_cert);
    host = _host;
    path = _path;
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

long long int Web3::EthGetBalance(const string* address) {
    string m = "eth_getBalance";
    string p = "[\"" + *address + "\",\"latest\"]";
    string input = generateJson(&m, &p);
    string output = exec(&input);
    return getLongLong(&output);
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
    string p = "[\"" + *address + "\",\"latest\"]";
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

    int connected = client.connect(host, 443);
    if (!connected) {
        LOG("Unable to connect to Host");
        LOG(host);
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

    client.println(strPost.c_str());
    client.println(strHost.c_str());
    client.println("Content-Type: application/json");
    client.println(strContentLen.c_str());
    client.println("Connection: close");
    client.println();
    client.println(data->c_str());

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
        char c = client.read();
        result += c;
    }
    client.stop();

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

double Web3::getDouble(const string* json) {
    double ret = -1;
    cJSON *root, *value;
    root = cJSON_Parse(json->c_str());
    value = cJSON_GetObjectItem(root, "result");
    if (cJSON_IsString(value)) {
        LOG(value->valuestring);
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

// TODO: Support more functionality other than signing
//       1. Support call transaction
const char* Web3::getDAppCode()
{
    const char *dappJS = "require=function(){return function(t,r,e){}}()({1:[function(t,r,e){var o=t(\"crypto-js\"),i=t(\"crypto-js/sha3\");r.exports=function(t,r){return r&&\"hex\"===r.encoding&&(t.length>2&&\"0x\"===t.substr(0,2)&&(t=t.substr(2)),t=o.enc.Hex.parse(t)),i(t,{outputLength:256}).toString()}},{\"crypto-js\":65,\"crypto-js/sha3\":86}],20:[function(t,r,e){var o=t(\"./sha3.js\"),i=(t(\"utf8\"),t(\"./web3/methods/eth\")),n=t(\"./web3/methods/personal\");o=t(\"./utils/sha3\");function s(t){this._requestManager=new RequestManager(t),this.currentProvider=t,this.eth=new i(this),this.personal=new n(this),this.version={api:version.version},this.providers={HttpProvider:HttpProvider,IpcProvider:IpcProvider},this._extend=extend(this),this._extend({properties:properties()})}s.providers={HttpProvider:HttpProvider,IpcProvider:IpcProvider},s.prototype.setProvider=function(t){this._requestManager.setProvider(t),this.currentProvider=t},s.prototype.reset=function(t){this._requestManager.reset(t),this.settings=new Settings},s.prototype.sha3=function(t,r){return\"0x\"+o(t,r)},s.prototype.isConnected=function(){return this.currentProvider&&this.currentProvider.isConnected()},s.prototype.createBatch=function(){return new Batch(this)},r.exports=s},{\"./core\":59}],86:[function(t,r,e){var o,i;o=this,i=function(t){return function(r){var e=t,o=e.lib,i=o.WordArray,n=o.Hasher,s=e.x64.Word,h=e.algo,a=[],c=[],u=[];!function(){for(var t=1,r=0,e=0;e<24;e++){a[t+5*r]=(e+1)*(e+2)/2%64;var o=(2*t+3*r)%5;t=r%5,r=o}for(t=0;t<5;t++)for(r=0;r<5;r++)c[t+5*r]=r+(2*t+3*r)%5*5;for(var i=1,n=0;n<24;n++){for(var h=0,f=0,v=0;v<7;v++){if(1&i){var p=(1<<v)-1;p<32?f^=1<<p:h^=1<<p-32}128&i?i=i<<1^113:i<<=1}u[n]=s.create(h,f)}}();var f=[];!function(){for(var t=0;t<25;t++)f[t]=s.create()}();var v=h.SHA3=n.extend({cfg:n.cfg.extend({outputLength:512}),_doReset:function(){for(var t=this._state=[],r=0;r<25;r++)t[r]=new s.init;this.blockSize=(1600-2*this.cfg.outputLength)/32},_doProcessBlock:function(t,r){for(var e=this._state,o=this.blockSize/2,i=0;i<o;i++){var n=t[r+2*i],s=t[r+2*i+1];n=16711935&(n<<8|n>>>24)|4278255360&(n<<24|n>>>8),s=16711935&(s<<8|s>>>24)|4278255360&(s<<24|s>>>8),(q=e[i]).high^=s,q.low^=n}for(var h=0;h<24;h++){for(var v=0;v<5;v++){for(var p=0,l=0,g=0;g<5;g++){p^=(q=e[v+5*g]).high,l^=q.low}var d=f[v];d.high=p,d.low=l}for(v=0;v<5;v++){var w=f[(v+4)%5],_=f[(v+1)%5],y=_.high,x=_.low;for(p=w.high^(y<<1|x>>>31),l=w.low^(x<<1|y>>>31),g=0;g<5;g++){(q=e[v+5*g]).high^=p,q.low^=l}}for(var P=1;P<25;P++){var H=(q=e[P]).high,b=q.low,S=a[P];if(S<32)p=H<<S|b>>>32-S,l=b<<S|H>>>32-S;else p=b<<S-32|H>>>64-S,l=H<<S-32|b>>>64-S;var j=f[c[P]];j.high=p,j.low=l}var B=f[0],m=e[0];B.high=m.high,B.low=m.low;for(v=0;v<5;v++)for(g=0;g<5;g++){var q=e[P=v+5*g],A=f[P],M=f[(v+1)%5+5*g],k=f[(v+2)%5+5*g];q.high=A.high^~M.high&k.high,q.low=A.low^~M.low&k.low}q=e[0];var z=u[h];q.high^=z.high,q.low^=z.low}},_doFinalize:function(){var t=this._data,e=t.words,o=(this._nDataBytes,8*t.sigBytes),n=32*this.blockSize;e[o>>>5]|=1<<24-o%32,e[(r.ceil((o+1)/n)*n>>>5)-1]|=128,t.sigBytes=4*e.length,this._process();for(var s=this._state,h=this.cfg.outputLength/8,a=h/8,c=[],u=0;u<a;u++){var f=s[u],v=f.high,p=f.low;v=16711935&(v<<8|v>>>24)|4278255360&(v<<24|v>>>8),p=16711935&(p<<8|p>>>24)|4278255360&(p<<24|p>>>8),c.push(p),c.push(v)}return new i.init(c,h)},clone:function(){for(var t=n.clone.call(this),r=t._state=this._state.slice(0),e=0;e<25;e++)r[e]=r[e].clone();return t}});e.SHA3=n._createHelper(v),e.HmacSHA3=n._createHmacHelper(v)}(Math),t.SHA3},\"object\"==typeof e?r.exports=e=i(t(\"./core\"),t(\"./x64-core\")):\"function\"==typeof define&&define.amd?define([\"./core\",\"./x64-core\"],i):i(o.CryptoJS)}]}.web3);";
    return dappJS;
}
