#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WifiServer.h>
#include <Web3.h>
#include <Contract.h>
#include <Crypto.h>
#include <vector>
#include <string>
#include <Util.h>

// This sample should work straight 'out of the box' after you plug in your WiFi credentials

const char *ssid = "<YOUR SSID>";
const char *password = "<YOUR WiFi PASSWORD>";
const char *INFURA_HOST = "kovan.infura.io";
const char *INFURA_PATH = "/v3/c7df4c29472d4d54a39f7aa78f146853";    //this is an anonymous infura public API key for courtesy testing, please don't use it for production
#define NATIVE_ETH_TOKENS "Kovan ETH"                                //if you switch chains you might want to change this
#define ERC875CONTRACT "0x0b70dd9f8ada11eee393c8ab0dd0d3df6a172876"  //an ERC875 token contract on Kovan
#define ERC20CONTRACT  "0xb06d72a24df50d4e2cac133b320c5e7de3ef94cb"  //and ERC20 token contract on Kovan
#define USERACCOUNT "0x835bb27deec61e1cd81b3a2feec9fbd76b15971d"     //a user account that holds Kovan ETH and balances of tokens in the two above contracts 

Web3 web3(INFURA_HOST, INFURA_PATH);
int wificounter = 0;

void queryERC875Balance(const char* Address, const char* ERC875ContractAddress);
void queryERC20Balance(String userAddress, const char* ERC875ContractAddress);
void setup_wifi();

void setup() 
{
    Serial.begin(115200); //ensure you set your Arduino IDE port config or platformio.ini with monitor_speed = 115200
    setup_wifi();

    string userAddress = USERACCOUNT;

    long long int balance = web3.EthGetBalance(&userAddress);
    char tmp[32];
    memset(tmp, 0, 32);
    sprintf(tmp, "%lld", balance);
    string val = string(tmp);

    Serial.println("eth_getBalance");
    //convert balance to double value
    string accountBalanceValue = Util::ConvertDecimal(18, &val);
    double bal = atof(accountBalanceValue.c_str());
    Serial.print(bal);
    Serial.print(" ");
    Serial.println(NATIVE_ETH_TOKENS);

    Serial.println("CALLING ERC875");

    queryERC875Balance(USERACCOUNT, ERC875CONTRACT);
    queryERC20Balance(USERACCOUNT, ERC20CONTRACT);

    Serial.println("FINISHED");
}


void loop() 
{
    // put your main code here, to run repeatedly.
}


/* This routine is specifically geared for ESP32 perculiarities */
/* You may need to change the code as required */
/* It should work on 8266 as well */
void setup_wifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);

        WiFi.begin(ssid, password);
    }

    wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 10)
    {
        for (int i = 0; i < 500; i++)
        {
            delay(1);
        }
        Serial.print(".");
        wificounter++;
    }

    if (wificounter >= 10)
    {
        Serial.println("Restarting ...");
        ESP.restart(); //targetting 8266 & Esp32 - you may need to replace this
    }

    delay(10);

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

/* Query balance of ERC875 tokens */
void queryERC875Balance(const char *userAddress, const char* ContractAddress)
{
    // transaction
    Contract contract(&web3, ContractAddress);

    String myAddr = userAddress;

    String func = "balanceOf(address)";
    Serial.println("Start");
    string param = contract.SetupContractData(func.c_str(), &myAddr);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    Serial.print("Balance of Contract ");
    Serial.println(ContractAddress);
    Serial.print("for user: ");
    Serial.println(myAddr.c_str());
    Serial.println();

    //break down the result
    vector<string> *vectorResult = Util::InterpretVectorResult(&result);
    int count = 1;
    char buffer[128];
    for (auto itr = vectorResult->begin(); itr != vectorResult->end(); itr++)
    {
        snprintf(buffer, 128, "%d: %s", count++, itr->c_str());
        Serial.println(buffer);
    }

    delete(vectorResult);

    //Call contract name function
    param = contract.SetupContractData("name()", &userAddress);
    result = contract.ViewCall(&param);
    string contractName = web3.getString(&result);
    Serial.println("NAME: ");
    // recover actual string name
    string interpreted = Util::InterpretStringResult(contractName.c_str());
    Serial.println(interpreted.c_str());
}

/* Query balance of ERC875 tokens */
void queryERC20Balance(String userAddress, const char *ContractAddress)
{
    Serial.println("ERC20 token fucntions");
    // query balance of account
    Contract contract(&web3, ContractAddress);
    string func = "balanceOf(address)";
    string param = contract.SetupContractData(func.c_str(), &userAddress);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    Serial.print("Balance of Contract ");
    Serial.println(ContractAddress);
    Serial.println("for user: ");
    Serial.println(userAddress);
    Serial.println();

    Serial.println(result.c_str());
    //Now go through the process to determine the token balance
    string returnVal = web3.getString(&result);
    Serial.println(returnVal.c_str());
    //ERC20 token returns a 32 byte BigInteger Hex, need to convert to decimal
    string val = Util::ConvertBase(16, 10, returnVal.c_str()); //convert base 16 to base 10
    Serial.println(val.c_str());
    //now get the value

    //get decimals for contract
    param = contract.SetupContractData("decimals()", &userAddress);
    result = contract.ViewCall(&param);
    int decimals = web3.getInt(&result);
    Serial.println("Decimals: ");
    Serial.println(decimals);

    //now find the actual double value of the token balance
    string actualTokenBalance = Util::ConvertDecimal(decimals, &val);
    double bal = atof(actualTokenBalance.c_str());
    Serial.println("Actual ERC20 balance: ");
    Serial.println(bal);

    param = contract.SetupContractData("name()", &userAddress);
    result = contract.ViewCall(&param);
    string contractName = web3.getString(&result);
    Serial.println("NAME: ");

    // recover actual string name
    string interpreted = Util::InterpretStringResult(contractName.c_str());
    Serial.println(interpreted.c_str());

    param = contract.SetupContractData("symbol()", &userAddress);
    result = contract.ViewCall(&param);
    string contractSymbol = web3.getString(&result);
    Serial.println("SYMBOL: ");

    // recover actual string symbol
    interpreted = Util::InterpretStringResult(contractSymbol.c_str());
    Serial.println(interpreted.c_str());
}
