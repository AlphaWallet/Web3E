# Web3E Ethereum for Embedded devices running Arduino framework

Version 0.9

Originally forked https://github.com/kopanitsa/web3-arduino but with almost a complete re-write it is a new framework entirely.

Starting from a simple requirement - write a dapp capable of running on an ESP32 which can serve as a security door entry system, the hunt began. On finding kopanitsa's excellent start there was an initial anchor point but it was quickly determined that the framework was far from capable of performing what it needed to, and had a great many flaws and missing areas.

- Library has a Ready-to-Go DApp injection system that turns any device hosted site instantly into an Ethereum DApp with ECDSA crypto!
- Cryptography has been overhauled to use a cut-down version of Trezor Wallet's very skillfully optimised and production proven library.
- Transaction system has been overhauled to firstly fix, plug the holes and speed up the transaction flow.
- Usability has been overhauled.

Included in the package are three samples.

- Simple DApp. Shows the power of the library to create a DApp server truly embedded in the device. The on-board cryptography engine can fully interact with user input. Signing, recovery/verification takes milliseconds on ESP32.
- Query Wallet balances, Token balances and for the first time Non-Fungible-Token (NFT) balances.
- Push transactions, showing token transfer of ERC20 and ERC875 tokens.

The push transaction sample requires a little work to get running. You have to have an Ethereum wallet, some testnet ETH, the private key for that testnet eth, and then create some ERC20 and ERC875 tokens in the account.

Coming soon:

- Security door using NFT access (currently live at AlphaWallet office!).
- ERC1155 balance enquiry.
