# Web3E Ethereum for Embedded devices

<img align="right" src="https://raw.githubusercontent.com/JamesSmartCell/Release-Test/master/Web3-Esmall.png">

## Version 0.9

Web3E is a functional but still in development Web3 framework for Embedded devices running Arduino. Tested mainly on ESP32 and working on ESP8266. Also included is a rapid development DApp injector to convert your embedded server into a fully integrated Ethereum DApp. 

Starting from a simple requirement - write a DApp capable of running on an ESP32 which can serve as a security door entry system. Some brave attempts can be found in scattered repos but ultimately even the best are just dapp veneers or have ingeneous and clunky hand-rolled communication systems like the Arduino wallet attempts.

What is required is a method to write simple, fully embedded DApps which give you a zero infrastucture and total security solution.
It is possible that as Ethereum runs natively on embedded devices a new revolution in the blockchain saga will begin. Now you have the ability to write a fully embedded DApp that gives you the seurity and flexibility of Ethereum in an embedded device.

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
- [AlphaWallet](https://www.awallet.io)
- [Testnet Eth](https://faucet.kovan.network). Visit this site on the DApp browser.
- [Mint some ERC875 tokens](https://alpha-wallet.github.io/ERC875-token-factory/index.html). Visit here on your DApp browser.
- Take a note of the contract address. Copy/paste contract address into source code inside the 'STORMBIRD_CONTRACT' define.
- Build and deploy the sample to your Arduino framework device.
- Use the transfer or MagicLink on AlphaWallet to give out the tokens.

## Included in the package are four samples

- Simple DApp. Shows the power of the library to create a DApp server truly embedded in the device. The on-board cryptography engine can fully interact with user input. Signing, recovery/verification takes milliseconds on ESP32.
- Query Wallet balances, Token balances and for the first time Non-Fungible-Token (NFT) balances.
- Push transactions, showing token transfer of ERC20 and ERC875 tokens.
- Send Eth, showing how to send native eth.

The push transaction sample requires a little work to get running. You have to have an Ethereum wallet, some testnet ETH, the private key for that testnet eth, and then create some ERC20 and ERC875 tokens in the account.

## Usage

## Ethereum transaction (ie send ETH to address):

```
// Setup Web3 and Contract with Private Key
...

uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&address); //obtain the next nonce
string result = contract.SendTransaction(nonceVal, <gas price>, <gas limit>, &toAddress, &weiValue, &emptyString);
```

## Query ETH balance:
```
long long int balance = web3.EthGetBalance(&userAddress); //obtain balance in Wei
char tmp[32];
sprintf(tmp, "%lld", balance);
string val = string(tmp);
string accountBalanceValue = Util::ConvertDecimal(18, &val); //Convert to Eth, Wei is eth * 10^18.
double bal = atof(accountBalanceValue.c_str());
```

## Send ERC20 Token:
```
string contractAddr = "<ERC20 Contract address>";
Contract contract(&web3, contractAddr.c_str());
contract.SetPrivateKey(<Your private key>);
uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&addr);
string toAddress = "<address to send to>";
string valueStr = "0x00";
string p = contract.SetupContractData("transfer(address,uint256)", &toAddress, 500); //ERC20 function plus params
result = contract.SendTransaction(nonceVal, <gas price>, <gas limit>, &contractAddr, &valueStr, &p);
```


Originally forked https://github.com/kopanitsa/web3-arduino but with almost a complete re-write it is a new framework entirely.

Libraries used:
- Web3 Arduino https://github.com/kopanitsa/web3-arduino - skeleton of framework.
- Trezor Crypto https://github.com/trezor/trezor-crypto - ECDSA sign, recover, verify, keccak256
- cJSON https://github.com/DaveGamble/cJSON

Coming soon:

- Security door using NFT access (currently live at AlphaWallet office!).
- ERC1155 balance enquiry.
- Use Templates in library code for more flexible development.

# Donations
If you support the cause, we could certainly use donations to help fund development:

0xbc8dAfeacA658Ae0857C80D8Aa6dE4D487577c63
