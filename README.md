# Web3E Ethereum for Embedded devices

<img align="right" src="https://raw.githubusercontent.com/JamesSmartCell/Release-Test/master/Web3-Esmall.png">

## Version 1.10

Web3E is a fully functional Web3 framework for Embedded devices running Arduino. Web3E now has methods which allow you to use TokenScript in your IoT solution for rapid deployment. Tested mainly on ESP32 and working on ESP8266. Also included is a rapid development DApp injector to convert your embedded server into a fully integrated Ethereum DApp. 

Starting from a simple requirement - write a DApp capable of running on an ESP32 which can serve as a security door entry system. Some brave attempts can be found in scattered repos but ultimately even the best are just dapp veneers or have ingeneous and clunky hand-rolled communication systems like the Arduino wallet attempts.

What is required is a method to write simple, fully embedded DApps which give you a zero infrastucture and total security solution.
It is possible that as Ethereum runs natively on embedded devices a new revolution in the blockchain saga will begin. Now you have the ability to write a fully embedded DApp that gives you the seurity and flexibility of Ethereum in an embedded device.

## New Features

- TokenScript/API interface [TokenScript](https://tokenscript.org)
- uint256 class added to correctly handle Ethereum types.
- usability methods added for converting between doubles and Wei values.
- usability methods added for displaying Wei values as doubles.
- random number generation uses Mersenne Twister.
- memory usage improved.

## Features

- Web3E now has a streamlined [TokenScript](https://tokenscript.org) interface for smooth integration with AlphaWallet, and other TokenScript powered wallet.
- Web3E has a Ready-to-Go DApp injection system that turns any device hosted site instantly into an Ethereum DApp with ECDSA crypto!
- Cryptography has been overhauled to use a cut-down version of Trezor Wallet's heavily optimised and production proven library.
- Transaction system is fully optimised and has been tested on ERC20 and ERC875 contracts.
- Usability has been a priority.

## Installation

- It is recommended to use [Platformio](https://platformio.org/install/) for best experience. Web3E is now part of the Platformio libraries so no need to clone the repo.
- Using Web3E is a one step process:
    1. Create a new project in Platformio and edit the platformio.ini so it looks similar to:

```
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; Serial Monitor options
monitor_speed = 115200

lib_deps =
  # Using a library name
  Web3E
```

## Example TokenScript flow

- Two API end points are exposed on the embedded device, '/api/getChallenge' and 'api/checkSignature?sig=<sig>'
- Device creates a challenge string, lets say 'Oranges 22853'.
- Tokenscript enabled phone runs embedded JavaScript that does a 'fetch' on 'getChallenge', returning 'Oranges 22853'.
- User is asked to sign this message using Ethereum SignPersonalMessage, with the same key that owns an entry token.
- Signature is sent back to embedded device using 'fetch' on 'api/checkSignature?sig=<signature of 'Oranges 22853'>'.
- Device ECRecover's an Ethereum Address from the signature using the current challenge.
- If address from ECRecover holds a token then open the door.
- Report pass/fail to callee.

The advantage of using TokenScript rather than a dapp is evident from looking at the code example. You will have a very nice user interface defined with html/css with the calling code written in small JavaScript functions. The user would quickly find the entry token in their wallet.

## Example Web3E DApp flow

- Device creates a challenge string, lets say 'Oranges 22853'. 
- Sign button containing a JavaScript Web3 call will instruct the wallet browser to ask the user to use their private key to sign 'Oranges 22853'. 
- After user signs, the signature and the user's address are passed back into the code running on the firmware.
- Web3E can perform ECRecover on the signature and the challenge string it created.
- The device code compares the recovered address with the address from the user. If they match then we have verified the user holds the private key for that address.
- Web3E can now check for specific permission tokens held by the user address. If the tokens are present the user has permission to operate whatever is connected to the device, could be a security door, safe, the office printer, a shared bitcoin hardware wallet etc.
- All operations are offchain ie gasless, but using on-chain attestations which an owner can issue at will.

## AlphaWallet Security Door

https://github.com/alpha-wallet/Web3E-Application

Full source code for the [system active at the AlphaWallet office](https://www.youtube.com/watch?v=D_pMOMxXrYY). To get it working you need:
- [Platformio](https://platformio.org/)
- [AlphaWallet](https://www.alphawallet.com)
- [Testnet Eth Kovan](https://faucet.kovan.network). Visit this site on the DApp browser.
- [Testnet Eth Goerli](https://goerli-faucet.slock.it/). Visit this site on your DApp browser.
Choose NonFungible token standard ERC721 or ERC875. Note the samples are written for ERC875 so you may need to adapt them for the ERC721 balance check.
- [Mint some ERC721 tokens](https://mintable.app/create) Visit here on your DApp browser.
- [Mint some ERC875 tokens](https://tf.alphawallet.com) Visit here on your DApp browser.
- Take a note of the contract address. Copy/paste contract address into source code inside the 'STORMBIRD_CONTRACT' define.
- Build and deploy the sample to your Arduino framework device.
- Use the transfer or MagicLink on AlphaWallet to give out the tokens.

## Included in the package are seven samples

- Simple DApp. Shows the power of the library to create a DApp server truly embedded in the device. The on-board cryptography engine can fully interact with user input. Signing, recovery/verification takes milliseconds on ESP32.
- Query Wallet balances, Token balances and for the first time Non-Fungible-Token (NFT) balances.
- Push transactions, showing token transfer of ERC20 and ERC875 tokens.
- Send Eth, showing how to send native eth.
- Wallet Bridge 1: Introduction to simple TokenScript connection to wallet.
- Wallet Bridge 2: Use simple authentication to check pre-defined addresses.
- Wallet Bridge 3: Use Ethereum Tokens as security attestations to interct with your IoT directly via the wallet.

The push transaction sample requires a little work to get running. You have to have an Ethereum wallet, some testnet ETH, the private key for that testnet eth, and then create some ERC20 and ERC875 tokens in the account.

## Usage
See Wallet Bridge 1, 2 and 3 examples for complete source

## Standard: Using ScriptProxy Bridge
### This style of connection will work in almost every situation, from inside or outside of the WiFi connection
In this usage pattern, your IoT device will connect to a proxy server which provides a bridge to the TokenScript running on your wallet. 
The source code for the proxy server can be found here: [Script Proxy](https://github.com/AlphaWallet/Web3E-Application/tree/master/ScriptProxy)

- Set up API routes
```
    const char *apiRoute = "api/";
    enum APIRoutes {   
        api_getChallenge, 
        api_checkSignature, 
        api_End };
					
    s_apiRoutes["getChallenge"] = api_getChallenge;
    s_apiRoutes["checkSignature"] = api_checkSignature;
    s_apiRoutes["end"] = api_End;
```

Declare UdpBridge, KeyID and Web3 in globals:

```
UdpBridge *udpConnection;
Web3 *web3;
KeyID *keyID;
```

Start UDP bridge after connecting to WiFi:

```
    udpConnection = new UdpBridge();
    udpConnection->setKey(keyID, web3);
    udpConnection->startConnection();
```

Within your loop() check for API call:

```
	udpConnection->checkClientAPI(&handleAPI);
```

Handle API call in the callback:

```
void handleAPI(APIReturn *apiReturn, UdpBridge *udpBridge, int methodId)
{
    switch (s_apiRoutes[apiReturn->apiName.c_str()])
    {
	case api_getChallenge:
		Serial.println(currentChallenge.c_str());
		udpBridge->sendResponse(currentChallenge, methodId);
		break;
	case api_checkSignature:
		{
			//EC-Recover address from signature and challenge
			string address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &currentChallenge);  
			//Check if this address has our entry token
			boolean hasToken = QueryBalance(&address);
			updateChallenge(); //generate a new challenge after each check
			if (hasToken)
			{
				udpBridge->sendResponse("pass", methodId);
				OpenDoor(); //Call your code that opens a door or performs the required 'pass' action
			}
			else
			{
				udpBridge->sendResponse("fail: doesn't have token", methodId);
			}
		}
                break;	
```


## Advanced: Direct TCP connection
### Use this if you have admin control of your WiFi Router, as you need to set up port forwarding to access the unit from outside your WiFi
In this usage pattern, the TokenScript running on the wallet will connect directly to the IoT device. Notice that this means your IoT is directly accessible to the internet, which may be susceptible to exploit.

- Declare the TCP server in globals:
```
WiFiServer server(8082);
```
- Ensure your Device is locked to a fixed IP Address for port forwarding (adjust local IP address as required):
```
IPAddress ipStat(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);
WiFi.config(ipStat, gateway, subnet, dns, dns);
```


- Set up API routes
```
    const char *apiRoute = "api/";
    enum APIRoutes {   
        api_getChallenge, 
        api_checkSignature, 
        api_End };
					
    s_apiRoutes["getChallenge"] = api_getChallenge;
    s_apiRoutes["checkSignature"] = api_checkSignature;
    s_apiRoutes["end"] = api_End;
```
- Listen for API call:
```
    WiFiClient c = server.available(); // Listen for incoming clients
    ScriptClient *client = (ScriptClient*) &c;

    if (*client)
    {
        Serial.println("New Client.");
        client->checkClientAPI(apiRoute, &handleAPI); //method handles connection close etc.
    }
```
- Handle API return:
```
void handleAPI(APIReturn *apiReturn, ScriptClient *client)
{
    switch(s_apiRoutes[apiReturn->apiName.c_str()])
    {
        case api_getChallenge:
            client->print(currentChallenge.c_str());
            break;
        case api_checkSignature:
            {
				//EC-Recover address from signature and challenge
                string address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &currentChallenge);  
				//Check if this address has our entry token
                boolean hasToken = QueryBalance(&address);
                updateChallenge(); //generate a new challenge after each check
                if (hasToken)
                {
                    client->print("pass");
                    OpenDoor(); //Call your code that opens a door or performs the required 'pass' action
                }
                else
                {
                    client->print("fail: doesn't have token");
                }
            }
            break;
    }
```

## Ethereum transaction (ie send ETH to address):

```
// Setup Web3 and Contract with Private Key
...

Contract contract(&web3, "");
contract.SetPrivateKey(PRIVATE_KEY);
uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&address); //obtain the next nonce
uint256_t weiValue = Util::ConvertToWei(0.25, 18); //send 0.25 eth
unsigned long long gasPriceVal = 1000000000ULL;
uint32_t  gasLimitVal = 90000;
string emptyString = "";
string toAddress = "0xC067A53c91258ba513059919E03B81CF93f57Ac7";
string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &toAddress, &weiValue, &emptyString);
```

## Query ETH balance:
```
uint256_t balance = web3.EthGetBalance(&address); //obtain balance in Wei
string balanceStr = Util::ConvertWeiToEthString(&balance, 18); //get string balance as Eth (18 decimals)
```

## Query ERC20 Balance:
```
string address = string("0x007bee82bdd9e866b2bd114780a47f2261c684e3");
Contract contract(&web3, "0x20fe562d797a42dcb3399062ae9546cd06f63280"); //contract is on Ropsten

//Obtain decimals to correctly display ERC20 balance (if you already know this you can skip this step)
string param = contract.SetupContractData("decimals()", &address);
string result = contract.ViewCall(&param);
int decimals = web3.getInt(&result);

//Fetch the balance in base units
param = contract.SetupContractData("balanceOf(address)", &address);
result = contract.ViewCall(&param);

uint256_t baseBalance = web3.getUint256(&result);
string balanceStr = Util::ConvertWeiToEthString(&baseBalance, decimals); //convert balance to double style using decimal places
```

## Send ERC20 Token:
```
string contractAddr = "0x20fe562d797a42dcb3399062ae9546cd06f63280";
Contract contract(&web3, contractAddr.c_str());
contract.SetPrivateKey(<Your private key>);

//Get contract name
string param = contract.SetupContractData("name()", &addr);
string result = contract.ViewCall(&param);
string interpreted = Util::InterpretStringResult(web3.getString(&result).c_str());
Serial.println(interpreted.c_str());

//Get Contract decimals
param = contract.SetupContractData("decimals()", &addr);
result = contract.ViewCall(&param);
int decimals = web3.getInt(&result);
Serial.println(decimals);

unsigned long long gasPriceVal = 22000000000ULL;
uint32_t  gasLimitVal = 4300000;

//amount of erc20 token to send, note we use decimal value obtained earlier
uint256_t weiValue = Util::ConvertToWei(0.1, decimals);

//get nonce
uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&addr);
string toAddress = "0x007bee82bdd9e866b2bd114780a47f2261c684e3";
string valueStr = "0x00";

//Setup contract function call
string p = contract.SetupContractData("transfer(address,uint256)", &toAddress, &weiValue); //ERC20 function plus params

//push transaction to ethereum
result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &contractAddr, &valueStr, &p);
string transactionHash = web3.getString(&result);
```

Originally forked https://github.com/kopanitsa/web3-arduino but with almost a complete re-write it is a new framework entirely.

Libraries used:
- Web3 Arduino https://github.com/kopanitsa/web3-arduino - skeleton of framework.
- Trezor Crypto https://github.com/trezor/trezor-crypto - ECDSA sign, recover, verify, keccak256.
- cJSON https://github.com/DaveGamble/cJSON
- uint256 https://github.com/calccrypto/uint256_t - Lightweight uint256 implementation perfect for embedded devices.
- Mersenne Twister https://github.com/MersenneTwister-Lab/TinyMT.git - For the most optimal random number generation for embedded.

Coming soon:

- Security door using NFT access (currently live at AlphaWallet office!).
- ERC1155 balance enquiry.
- Use Templates in library code for more flexible development.

# Donations
If you support the cause, we could certainly use donations to help fund development:

0xbc8dAfeacA658Ae0857C80D8Aa6dE4D487577c63
