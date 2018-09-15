//
// Created by Okada, Takahiro on 2018/02/03.
//
// Reference:
//     Arduino sample code (WiFiClientSecure)
//

#include <Arduino.h>
#include <WiFi.h>
#include "../Contract.h"
#include "../Conf.h"
#include "../Web3.h"

#define USE_SERIAL Serial

string host = INFURA_HOST;
string path = INFURA_PATH;
Web3 web3(&host, &path);

void testWeb3();
void testNet();
void testEth1();
void testEth2();
void testEth3();

void setup() {

    USE_SERIAL.begin(115200);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFi.begin(ENV_SSID, ENV_WIFI_KEY);

    // attempt to connect to Wifi network:
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        // wait 1 second for re-trying
        delay(1000);
    }

    USE_SERIAL.println("Connected");

//    testWeb3();
//    testNet();
//    testEth1();
    testEth2();
    testEth3();
}

void testWeb3() {
    string result = web3.Web3ClientVersion();
    USE_SERIAL.println("web3_ClientVersion");
    USE_SERIAL.println(result.c_str());

    string src = "0x68656c6c6f20776f726c64";
    result = web3.Web3Sha3(&src);
    USE_SERIAL.println("web3_sha3");
    USE_SERIAL.println(result.c_str()); // 0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad
}

void testNet() {
    int version = web3.NetVersion();
    USE_SERIAL.println("net_version");
    USE_SERIAL.println(version); // 4

    bool listening = web3.NetListening();
    USE_SERIAL.println("net_listening");
    if (listening) {
        USE_SERIAL.println("listening");
    } else{
        USE_SERIAL.println("not listening");
    }

    int peerCount = web3.NetPeerCount();
    USE_SERIAL.println("net_peerCount");
    USE_SERIAL.println(peerCount);
}

void testEth1() {
    char tmp[32];

    double version = web3.EthProtocolVersion();
    USE_SERIAL.println("eth_protocolVersion");
    USE_SERIAL.println(version);

    bool listening = web3.EthSyncing();
    USE_SERIAL.println("eth_syncing");
    if (listening) {
        USE_SERIAL.println("syncing");
    } else{
        USE_SERIAL.println("not syncing");
    }

    bool mining = web3.EthMining();
    USE_SERIAL.println("eth_mining");
    if (mining) {
        USE_SERIAL.println("mining");
    } else{
        USE_SERIAL.println("not mining");
    }

    double hashrate = web3.EthHashrate();
    USE_SERIAL.println("eth_hashrate");
    USE_SERIAL.println(hashrate);

    long long int gasPrice = web3.EthGasPrice();
    USE_SERIAL.println("eth_gasPrice");
    memset(tmp, 0, 32);
    sprintf(tmp, "%lld", gasPrice);
    USE_SERIAL.println(tmp);

    int blockNumber = web3.EthBlockNumber();
    USE_SERIAL.println("eth_blockNumber");
    memset(tmp, 0, 32);
    sprintf(tmp, "%d", blockNumber);
    USE_SERIAL.println(tmp);

    string address = "0xd7049ea6f47ef848C0Ad570dbA618A9f6e4Eb25C";
    long long int balance = web3.EthGetBalance(&address);
    USE_SERIAL.println("eth_getBalance");
    memset(tmp, 0, 32);
    sprintf(tmp, "%lld", balance);
    USE_SERIAL.println(tmp);

    int txcount = web3.EthGetTransactionCount(&address);
    USE_SERIAL.println("eth_getTransactionCount");
    memset(tmp, 0, 32);
    sprintf(tmp, "%d", txcount);
    USE_SERIAL.println(tmp);

}

void testEth2() {
    string from = "0xd7049ea6f47ef848c0ad570dba618a9f6e4eb25c";
    string to = "0x018968e41da1364e328499613a7e5a22904ad513";
    string value = "";
    string data = "0x30cc3597d7049ea6f47ef848c0ad570dba618a9f6e4eb25c";
    string result = "";
    result = web3.EthCall(&from, &to, 0, 0, &data, &data);
    USE_SERIAL.println("eth_call");

    string contract_address = "0x018968e41da1364e328499613a7e5a22904ad513";
    Contract contract(&web3, &contract_address);
    strcpy(contract.options.from,"0xd7049ea6f47ef848c0ad570dba618a9f6e4eb25c");
    strcpy(contract.options.gasPrice,"20000000000000");
    contract.options.gas = 5000000;
    string func = "buyCoin()";
    string param = contract.SetupContractData(&func);
    result = contract.Call(&param);
    USE_SERIAL.println(result.c_str());

    func = "hogeuint(uint8,uint16,uint32,uint256,uint)";
    param = contract.SetupContractData(&func, 1, 12345, 1003, 4, 5);
    result = contract.Call(&param);
    USE_SERIAL.println(result.c_str());

    func = "hogeint(int8,int16,int32,int256,int)";
    param = contract.SetupContractData(&func, -1, 12345, -1003, 4, -5);
    result = contract.Call(&param);
    USE_SERIAL.println(result.c_str());

    string address =  "0xd7049ea6f47ef848C0Ad570dbA618A9f6e4Eb25C";
    func = "hogeaddress(address)";
    param = contract.SetupContractData(&func, &address);
    result = contract.Call(&result);
    USE_SERIAL.println(result.c_str());

    func = "hogebool(bool,bool)";
    param = contract.SetupContractData(&func, true, false);

    result = contract.Call(&result);
    USE_SERIAL.println(result.c_str());

    string strParam = "Hello, world!";
    func = "hogestring(string)";
    param = contract.SetupContractData(&func, &strParam);
    result = contract.Call(&result);
    USE_SERIAL.println(result.c_str());

    func = "hogebytes(bytes5,bytes12)";
    param = contract.SetupContractData(&func, "hello", "123456789012");
    result = contract.Call(&result);
    USE_SERIAL.println(result.c_str());

}

void testEth3() {
    // raw transaction
    string param = "0xf8885a84086796cc832dc6c094e759aab0343e7d4c9e23ac5760a12ed9d9af442180a460fe47b100000000000000000000000000000000000000000000000000000000000000641ca0278d62b5bf2440fe1c572931a60970cccbaa167425575bcef80bf93f6bda6e7fa031efdd7520f72dc1eb8b619c1cf5058d8cbdd3581c5e16a40787e8887e8be257";
    string ret = web3.EthSendSignedTransaction(&param, param.size());
    USE_SERIAL.println("eth_sendRawTransaction");
    USE_SERIAL.println(ret.c_str());

    // transaction
    string contract_address = "0xe759aab0343e7d4c9e23ac5760a12ed9d9af4421";
    Contract contract(&web3, &contract_address);
    contract.SetPrivateKey(privateKey);
    string address = "0xd7049ea6f47ef848C0Ad570dbA618A9f6e4Eb25C";
    uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&address);

    uint32_t gasPriceVal = 141006540;
    uint32_t  gasLimitVal = 3000000;
    string toStr = "0xe759aab0343e7d4c9e23ac5760a12ed9d9af4421";
    string valueStr = "0x00";

    string func = "set(uint256)";
    string p = contract.SetupContractData(&func, 123);
    string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &toStr, &valueStr, &p);

}

void loop() {
    delay(5000);
}


