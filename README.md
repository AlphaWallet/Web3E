# Web3E Ethereum for Embedded devices running Arduino framework

Version 0.9

Web3E is a functional but still in development Web3 framework for Embedded devices running Arduino. Tested mainly on ESP32 and working on ESP8266. Also included is a rapid development DApp injector to convert your embedded server into a fully integrated Ethereum DApp. 

Starting from a simple requirement - write a DApp capable of running on an ESP32 which can serve as a security door entry system. There was an existing Web3 for Arduino which was an excellent start but seems to have been neglected. It is possible that as Ethereum runs natively on embedded devices a new revolution in the blockchain saga will begin.

- Web3E has a Ready-to-Go DApp injection system that turns any device hosted site instantly into an Ethereum DApp with ECDSA crypto!
- Cryptography has been overhauled to use a cut-down version of Trezor Wallet's heavily optimised and production proven library.
- Transaction system is fully optimised and has been tested on ERC20 and ERC875 contracts.
- Usability has been a priority.

Included in the package are three samples.

- Simple DApp. Shows the power of the library to create a DApp server truly embedded in the device. The on-board cryptography engine can fully interact with user input. Signing, recovery/verification takes milliseconds on ESP32.
- Query Wallet balances, Token balances and for the first time Non-Fungible-Token (NFT) balances.
- Push transactions, showing token transfer of ERC20 and ERC875 tokens.

The push transaction sample requires a little work to get running. You have to have an Ethereum wallet, some testnet ETH, the private key for that testnet eth, and then create some ERC20 and ERC875 tokens in the account.

Originally forked https://github.com/kopanitsa/web3-arduino but with almost a complete re-write it is a new framework entirely.

Libraries used:
- Web3 Arduino https://github.com/kopanitsa/web3-arduino - skeleton of framework.
- Trezor Crypto https://github.com/trezor/trezor-crypto - ECDSA sign, recover, verify, keccak256
- cJSON https://github.com/DaveGamble/cJSON

Coming soon:

- Security door using NFT access (currently live at AlphaWallet office!).
- ERC1155 balance enquiry.
