#include <WiFi.h>
#include <Web3.h>
#include <Contract.h>
#include <Util.h>

#define USE_SERIAL Serial
#define SIGNATURE_LENGTH 65

#define ENV_SSID     "Ten Forward"
#define ENV_WIFI_KEY "This place is happening man."
const string INFURA_HOST = "ropsten.infura.io";
const string INFURA_PATH2 = "/llyrtzQ3YhkdESt2Fzrk";
#define ADDRESS "0x007bee82bdd9e866b2bd114780a47f2261c684e3"
#define ROPSTEN_KEY "c64031ec35f5fc700264f6bb2d6342f63e020673f79ed70dbbd56fb8d46351ed"

Web3 web3(&INFURA_HOST, &INFURA_PATH2);

void eth_example();
char convertCharToHex(char ch);
void convertToBytes(BYTE *_dst, const char *_src, int length);
void queryBalance(string contractAddress);

static BYTE *privKey;

void setup() {
    USE_SERIAL.begin(115200);

    privKey = new BYTE[32];
    convertToBytes(privKey, ROPSTEN_KEY, 32);

    string contract_address = "0x66f08ca6892017a45da6fb792a8e946fcbe3d865";

    for(uint8_t t = 4; t > 0; t--) 
    {
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

    
    USE_SERIAL.println("ready ...");

    eth_example();
}

void loop() {
    // put your main code here, to run repeatedly:
}

void eth_example() 
{
    // bool listening = web3.EthSyncing();
    // USE_SERIAL.println("eth_syncing");
    // if (listening) {
    //     USE_SERIAL.println("syncing");
    // } else{
    //     USE_SERIAL.println("not syncing");
    // }

    // bool mining = web3.EthMining();
    // USE_SERIAL.println("eth_mining");
    // if (mining) {
    //     USE_SERIAL.println("mining");
    // } else{
    //     USE_SERIAL.println("not mining");
    // }

    // double hashrate = web3.EthHashrate();
    // USE_SERIAL.println("eth_hashrate");
    // USE_SERIAL.println(hashrate);

    // long long int gasPrice = web3.EthGasPrice();
    // USE_SERIAL.println("eth_gasPrice");
    // memset(tmp, 0, 32);
    // sprintf(tmp, "%lld", gasPrice);
    // USE_SERIAL.println(tmp);

    // int blockNumber = web3.EthBlockNumber();
    // USE_SERIAL.println("eth_blockNumber");
    // memset(tmp, 0, 32);
    // sprintf(tmp, "%d", blockNumber);
    // USE_SERIAL.println(tmp);

    // string address = ADDRESS;
    // long long int balance = web3.EthGetBalance(&address);
    // USE_SERIAL.println("eth_getBalance");
    // memset(tmp, 0, 32);
    // sprintf(tmp, "%lld", balance);
    // USE_SERIAL.println(tmp);

    // int txcount = web3.EthGetTransactionCount(&address);
    // USE_SERIAL.println("eth_getTransactionCount");
    // memset(tmp, 0, 32);
    // sprintf(tmp, "%d", txcount);
    // USE_SERIAL.println(tmp);

    //now try to get token balance
    queryBalance("0x66f08ca6892017a45da6fb792a8e946fcbe3d865");
}

void convertToBytes(BYTE *_dst, const char *_src, int length)
{
    if (_src[0] == '0' && _src[1] == 'x') _src += 2; //chop off 0x

    int i;
    for (i = 0; i < length; i++)
    {
        byte extract;
        char a = _src[2 * i];
        char b = _src[2 * i + 1];
        extract = convertCharToHex(a) << 4 | convertCharToHex(b);
        _dst[i] = extract;
        //Serial.println(_dst[i], HEX);
    }
}

bool hasValue(BYTE *value, int length)
{
    for (int i = 0; i < length; i++)
    {
        if ((*value++) != 0)
        {
            return true;
        }
    }

    return false;
}

void queryBalance(string contractAddress)
{
    // transaction
    Contract contract(&web3, &contractAddress);
    string func = "balanceOf(address)";
    string address = ADDRESS;
    string param = contract.SetupContractData(&func, &address);
    string result = contract.ViewCall(&param);

    BYTE tokenVal[32];

    bool hasToken = false;

    //break down the result
    vector<string> *vectorResult = contract.InterpretVectorResult(&result);
    for (auto itr = vectorResult->begin(); itr != vectorResult->end(); itr++)
    {
        convertToBytes(tokenVal, itr->c_str(), 32);
        if (hasValue(tokenVal, 32)) 
        {
            hasToken = true;
        }
    }

    if (hasToken)
    {
        Serial.println("Has token");
    }
    else
    {
        Serial.println("Doesn't have");
    }

    delete(vectorResult);
}

char convertCharToHex(char ch)
{
    char returnType;
    switch (ch)
    {
    case '0':
        returnType = 0;
        break;
    case '1':
        returnType = 1;
        break;
    case '2':
        returnType = 2;
        break;
    case '3':
        returnType = 3;
        break;
    case '4':
        returnType = 4;
        break;
    case '5':
        returnType = 5;
        break;
    case '6':
        returnType = 6;
        break;
    case '7':
        returnType = 7;
        break;
    case '8':
        returnType = 8;
        break;
    case '9':
        returnType = 9;
        break;
    case 'A':
    case 'a':
        returnType = 10;
        break;
    case 'B':
    case 'b':
        returnType = 11;
        break;
    case 'C':
    case 'c':
        returnType = 12;
        break;
    case 'D':
    case 'd':
        returnType = 13;
        break;
    case 'E':
    case 'e':
        returnType = 14;
        break;
    case 'F':
    case 'f':
        returnType = 15;
        break;
    default:
        returnType = 0;
        break;
    }
    return returnType;
}