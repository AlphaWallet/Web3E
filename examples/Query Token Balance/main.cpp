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
const char *INFURA_PATH = "/v3/<your Infura token>";    //this is an anonymous infura public API key for courtesy testing, please don't use it for production
#define NATIVE_ETH_TOKENS "Kovan ETH"                                //if you switch chains you might want to change this
#define ERC875CONTRACT "0x0b70dd9f8ada11eee393c8ab0dd0d3df6a172876"  //an ERC875 token contract on Kovan
#define ERC20CONTRACT  "0xb06d72a24df50d4e2cac133b320c5e7de3ef94cb"  //and ERC20 token contract on Kovan
#define USERACCOUNT "0x835bb27deec61e1cd81b3a2feec9fbd76b15971d"     //a user account that holds Kovan ETH and balances of tokens in the two above contracts 

Web3 web3(INFURA_HOST, INFURA_PATH);
int wificounter = 0;

void queryERC875Balance(const char* Address, const char* ERC875ContractAddress);
void queryERC20Balance(const char* Address, const char* ERC875ContractAddress);
void setup_wifi();
void TestERC20();
void QueryEthBalance(const char* Address);

void setup() 
{
    Serial.begin(115200); //ensure you set your Arduino IDE port config or platformio.ini with monitor_speed = 115200
    setup_wifi();

    string userAddress = USERACCOUNT;

	queryERC20Balance(ERC20CONTRACT, USERACCOUNT);
    queryERC875Balance(ERC875CONTRACT, USERACCOUNT);
}


void loop() 
{
    // put your main code here, to run repeatedly.
}

/* Query balance of ERC875 tokens */
void queryERC875Balance(const char* ContractAddress, const char *userAddress)
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

// Query balance of ERC20 token
void queryERC20Balance(const char* ContractAddress, const char *userAddress)
{
    Contract contract(&web3, contractAddr);
    String address = userAddress;
    string param = contract.SetupContractData("balanceOf(address)", &address);
    string result = contract.ViewCall(&param);

    param = contract.SetupContractData("decimals()", &address);
    result = contract.ViewCall(&param);
    int decimals = web3.getInt(&result);

    uint256_t baseBalance = web3.getUint256(&result);
    string balanceStr = Util::ConvertWeiToEthString(&baseBalance, decimals); //use decimals to calculate value, not all ERC20 use 18 decimals

    Serial.print("ERC20 Balance: ");
    Serial.println(balanceStr.c_str());
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