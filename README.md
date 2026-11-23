# Web3E Ethereum for Embedded devices

<img align="right" src="https://raw.githubusercontent.com/JamesSmartCell/Release-Test/master/Web3-Esmall.png">

## [![PlatformIO Registry](https://badges.registry.platformio.org/packages/alphawallet/library/Web3E.svg)](https://registry.platformio.org/libraries/alphawallet/Web3E)

What's New:
v1.46:
- Add firmware version reporting to TCP bridge login (opcode 0x09, servers accept both old and new).
- Add WifiStore EEPROM reader for persistent Wi-Fi credentials.
- CRITICAL FIX: Prevent EEPROM truncation bug that destroyed WifiStore data.
- Add WiFi connection diagnostics and watchdog with automatic reboot on timeout.
- Add startup watchdog and bounded API parsing (ported from GarageDoor).
- BREAKING: Default proxy changed to scriptproxy.percolate.one:8083 (old scriptproxy.smarttokenlabs.com no longer resolves). Devices on v1.45 or earlier will fail DNS. Use setProxy() or build flags to override.
- Fix compilation errors (WEB3E_PRINTln typo, missing getLastCommsTime).
- Add hardware E2E test tooling and telemetry verification.

v1.45:
- Fix case-sensitive include issues for Linux/PlatformIO builds.
- Fix `byte` type ambiguity with modern C++ compilers.
- Add CI testing to verify library packaging.
- Enable consumption via git URL with version tags.

v1.44:
- Fix type conversion ambiguity.
- Add required method in TcpBridge.

v1.43:
- Update readme text in August door sample to point to Project wizard at http://smarttokenlabs.duckdns.org/

v1.42:
- Use EIP-155 for sending transactions.
- Remove dependency on libm for converting doubles.
- Fix various issues.
- Rework examples to ensure they work and improve example code quality.

v1.41:
- Allow multiple dynamic params on function calls.
- Fix/allow unlimited length params on TcpBridge.

v1.4:
- Add Sepolia testnet.
- Ensure Polygon and Mumbai are working correctly.
- Optimise and fix contract call result handling.
- Handle String return correctly.

v1.34:
- Allow long params in ScriptBridge.
- Correctly RLP encode (bytes) params.
- Add utility function for decoding responses.

v1.33:
- Fix & simplify node setup - only need to specify numeric ChainID or use the pre-definition eg ```Web3 web3 = new Web3(GOERLI_ID)```.
- Fix & update Node certificates.
- Add Sepolia, Arbirtum and Optimism Goerli. 

For a working demonstration of TokenScript and Web3E please try minting an ERC5169/TokenScript powered ERC721 NFT here: http://smarttokenlabs.duckdns.org
You will need AlphaWallet on your mobile phone; simply replace the main.cpp in the project here: https://github.com/TokenScript/TokenScriptTestContracts/tree/master/firmware with the generated project which the Token wizard generates.

Web3E is a fully functional Web3 framework for Embedded devices running Arduino. Web3E now has methods which allow you to use TokenScript in your IoT solution for rapid deployment. Tested mainly on ESP32 and working on ESP8266. Also included is a rapid development DApp injector to convert your embedded server into a fully integrated Ethereum DApp. 

Starting from a simple requirement - write a DApp capable of running on an ESP32 which can serve as a security door entry system. Some brave attempts can be found in scattered repos but ultimately even the best are just dapp veneers or have ingenous and clunky hand-rolled communication systems like the Arduino wallet attempts.

What is required is a method to write simple, fully embedded DApps which give you a zero infrastructure and total security solution.
It is possible that as Ethereum runs natively on embedded devices a new revolution in the blockchain saga will begin. Now you have the ability to write a fully embedded DApp that gives you the security and flexibility of Ethereum in an embedded device.

## New Features

- Fixed transaction send to use EIP-155.
- Reduce firmware size footprint.
- Reduce dependency on external libraries.
- Produce Interface wizard which creates an NFT, writes and uploads TokenScript to IPFS, links the TokenScript via EIP-5169 and writes the firmware.
  Wizard is here: http://smarttokenlabs.duckdns.org/ (Note that it is http, you need to trust the site!)
- Improve encoding and decoding for calling smartcontract functions.
- vastly improve SmartLayer comms with TokenScript.
- Test examples with latest library and improve code.

- Simplified Node connection.
- Add TCP Bridge.
- improve device key init.
- add August lock crypto interface sample.
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

- It is recommended to use [Platformio](https://platformio.org/install/) for best experience.
- Web3E can be consumed in three ways:

### Option 1: PlatformIO Registry (Stable Releases)

Web3E is part of the PlatformIO registry. Simply reference it by name:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200

lib_ldf_mode = deep
lib_deps =
  Web3E
```

### Option 2: Git URL with Version Tag (Recommended for Reproducible Builds)

Pin to a specific version for reproducible builds:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200

lib_ldf_mode = deep
lib_deps =
  https://github.com/AlphaWallet/Web3E.git#v1.46
```

### Option 3: Git URL with Commit Hash (For Testing or Specific Fixes)

Pin to a specific commit:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200

lib_ldf_mode = deep
lib_deps =
  https://github.com/AlphaWallet/Web3E.git#abc1234
```

Replace `abc1234` with the full commit hash you want to use.

**Please note: you may need to use 'lib_ldf_mode = deep' to locate some ESP32 libraries like 'EEPROM.h'**
**also: if you use many other libraries in your application you may need to add 'board_build.partitions = no_ota.csv' to fit your firmware**

### ESP32 Requirements

Web3E uses TLS/SSL for secure RPC connections, which requires adequate stack space and flash storage.

**Stack Size:** Increase the loop stack size to 16KB in your `platformio.ini`:

```ini
build_flags =
    -DARDUINO_LOOP_STACK_SIZE=16384
```

This prevents `Guru Meditation` errors with `Stack canary watchpoint triggered` messages during TLS handshakes.

**Flash Partition:** For larger applications (especially with network tests), use a bigger app partition:

```ini
board_build.partitions = huge_app.csv
```

Common partition schemes:
- `default.csv` - 1.3MB app (may be too small for complex apps)
- `min_spiffs.csv` - 1.9MB app, 190KB SPIFFS
- `huge_app.csv` - 3MB app, minimal SPIFFS (recommended for Web3E apps)

Without adequate flash space, you'll see: `Image length XXXXXX doesn't fit in partition length YYYYYY` and boot loops.

### RPC endpoints (no bundled node tables)

Web3E does **not** ship hardcoded per-chain RPC URLs or Infura keys. Construct
`Web3(chainId)` for crypto / KeyID / TokenScript bridge work, then obtain RPC
endpoints before any JSON-RPC call:

1. **Preferred:** `useCachedRpcPair(uninode)` — signed pair from universal-node for
   any chainId listed on [chainlist.org](https://chainlist.org) that uninode serves.
2. **Lab / private override:** `setRpcUrl("https://…")` — a single explicit HTTPS URL
   (e.g. your Infura/Alchemy project URL). Disables cached-pair mode.

HTTPS RPC connections use `setInsecure()` (encrypted without a pinned CA bundle).

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

## Chain IDs and RPC setup

`src/chainIds.h` lists common EVM chainId constants. Pass one to `Web3(chainId)`,
then configure RPC via universal-node (preferred) or an explicit URL:

``` C++
Web3* web3 = new Web3(SEPOLIA_ID);
KeyID* keyId = new KeyID(web3);
UniversalNode* uninode = new UniversalNode(web3, keyId);
uninode->syncTime();
web3->useCachedRpcPair(uninode);   // preferred — any chain uninode serves
// OR: web3->setRpcUrl("https://your-provider.example/v3/KEY");  // lab override
```

See **Cached public RPC pair** below and `examples/Cached RPC Pair`.

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
- [Testnet Eth Sepolia](https://sepoliafaucet.com/). Visit this site in the dapp browser or desktop with your browser plugin wallet.
- [Testnet Polygon Mumbai](https://mumbaifaucet.com/). Visit this site on your DApp browser - you'll need an ENS name to retrieve.
- Mint some ERC721 tokens. You can use OpenSea or mintable below
- [Mint some ERC721 tokens](https://mintable.app/create) Visit here on your DApp browser.
- [Mint some ERC20 tokens](https://www.smartcontracts.tools/token-generator/create/ethereum/) Visit here on your DApp browser.
- Take a note of the contract address. Copy/paste contract address into source code inside the 'DOOR_CONTRACT' or 'TOKEN_CONTRACT' define.
- Build and deploy the sample to your Arduino framework device.
- Use the transfer or MagicLink on AlphaWallet to give out the tokens.

## Included in the package are seven samples

- Simple DApp. Shows the power of the library to create a DApp server truly embedded in the device. The on-board cryptography engine can fully interact with user input. Signing, recovery/verification takes milliseconds on ESP32.
- Query Wallet balances, Token balances and for the first time Non-Fungible-Token (NFT) balances.
- Push transactions, showing token transfer of ERC20 and ERC875 tokens.
- Send Eth, showing how to send native eth.
- Wallet Bridge 1: Introduction to simple TokenScript connection to wallet.
- Wallet Bridge 2: Use simple authentication to check pre-defined addresses.
- Wallet Bridge 3: Use Ethereum Tokens as security attestations to interact with your IoT directly via the wallet.

The push transaction sample requires a little work to get running. You have to have an Ethereum wallet, some testnet ETH, the private key for that testnet eth, and then create some ERC20 and ERC875 tokens in the account.

## Usage
See Wallet Bridge 1, 2 and 3 examples for complete source

## 🔴 BREAKING CHANGE: Proxy Server Default Updated

**As of v1.46, the default proxy server has changed:**
- **Old default:** `scriptproxy.smarttokenlabs.com:8003` (NO LONGER RESOLVES)
- **New default:** `scriptproxy.percolate.one:8083` (TCP) / `:8083` (UDP)

**What this means:**
- Devices running **stock Web3E 1.45 or earlier** will fail to connect (DNS failure)
- GarageDoor and other production deployments using the old default **MUST update**
- Custom proxy configurations are unaffected

**How to override:**

```cpp
// At runtime
TcpBridge *tcpConnection = new TcpBridge();
tcpConnection->setProxy("your.proxy.com", 8083);  // Set before startConnection()
tcpConnection->startConnection();

// At compile time (platformio.ini)
build_flags =
    -DWEB3E_PROXY_HOST=\"your.proxy.com\"
    -DWEB3E_PROXY_PORT=8083

// Or use PROXY_HOST/PROXY_PORT (also supported)
build_flags =
    -DPROXY_HOST=\"your.proxy.com\"
    -DPROXY_PORT=8083
```

**UDP Bridge:**
```cpp
UdpBridge *udpConnection = new UdpBridge();
std::string host = "your.proxy.com";
udpConnection->setupConnection(host, 8083);  // Set before startConnection()
udpConnection->startConnection();
```

---

## Standard: Using ScriptProxy Bridge
### This style of connection will work in almost every situation, from inside or outside of the WiFi connection
In this usage pattern, your IoT device will connect to a proxy server which provides a bridge to the TokenScript running on your wallet. 
The source code for the proxy server can be found here: [SmartLayerProxy](https://github.com/JamesSmartCell/SmartLayerProxy)

- Set up API routes
``` C++
    const char *apiRoute = "api/";
    enum APIRoutes {   
        api_getChallenge, 
        api_checkSignature, 
        api_End };
					
    s_apiRoutes["getChallenge"] = api_getChallenge;
    s_apiRoutes["checkSignature"] = api_checkSignature;
    s_apiRoutes["end"] = api_End;
```

Declare TcpBridge, KeyID and Web3 in globals:

``` C++
TcpBridge *tcpConnection;
Web3 *web3;
KeyID *keyID;
```

Setup your Web node
``` C++
web3 = new Web3(MAINNET_ID);
```

Start UDP bridge after connecting to WiFi:

``` C++
    tcpConnection = new TcpBridge();
    tcpConnection->setKey(keyID, web3);
    tcpConnection->startConnection();
```

Within your loop() check for API call:

``` C++
    tcpConnection->checkClientAPI(&handleTCPAPI);
```

Handle API call in the callback:

``` C++
enum APIRoutes
{
  api_unknown,
  api_getChallenge,
  api_checkSignature,
  api_checkSignatureLock,
  api_checkMarqueeSig,
  api_end
};

std::map<std::string, APIRoutes> s_apiRoutes;

void Initialize()
{
  s_apiRoutes["getChallenge"] = api_getChallenge;
  s_apiRoutes["checkSignature"] = api_checkSignature;
  s_apiRoutes["checkSignatureLock"] = api_checkSignatureLock;
  s_apiRoutes["end"] = api_end;
}

std::string handleTCPAPI(APIReturn* apiReturn)
{
    switch (s_apiRoutes[apiReturn->apiName])
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
``` C++
WiFiServer server(8082);
```
- Ensure your Device is locked to a fixed IP Address for port forwarding (adjust local IP address as required):
``` C++
IPAddress ipStat(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);
WiFi.config(ipStat, gateway, subnet, dns, dns);
```


- Set up API routes
``` C++
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
``` C++
    WiFiClient c = server.available(); // Listen for incoming clients
    ScriptClient *client = (ScriptClient*) &c;

    if (*client)
    {
        Serial.println("New Client.");
        client->checkClientAPI(apiRoute, &handleAPI); //method handles connection close etc.
    }
```
- Handle API return:
``` C++
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

``` C++
// Setup Web3 and Contract with Private Key
...
web3 = new Web3(RINKEBY_ID);
Contract contract(web3, "");
contract.SetPrivateKey(PRIVATE_KEY);
uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&address); //obtain the next nonce
uint256_t weiValue = Util::ConvertToWei(0.25, 18); //send 0.25 eth
unsigned long long gasPriceVal = 1000000000ULL;
uint32_t  gasLimitVal = 90000;
string emptyString = "";
string toAddress = "0xC067A53c91258ba513059919E03B81CF93f57Ac7";
string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &toAddress, &weiValue, &emptyString);
```

## Query ETH balance:
``` C++
uint256_t balance = web3->EthGetBalance(&address); //obtain balance in Wei
string balanceStr = Util::ConvertWeiToEthString(&balance, 18); //get string balance as Eth (18 decimals)
```

## Signed reads via universal-node (MITM-resistant):
Use `UniversalNode` so `EthGetBalance` / `EthViewCall` (and other allowlisted reads) go through
`https://uninode.percolate.one` with EIP-191 request signing and response verification against a
pinned node signer. Requires NTP-synced time. See `examples/Universal Node RPC`.

``` C++
KeyID* keyId = new KeyID(web3);                 // EEPROM device key (bytes 0-32)
UniversalNode* uninode = new UniversalNode(web3, keyId);
uninode->syncTime();                            // required; fails clearly if not synced
web3->setUniversalNode(uninode);                // route read-only Web3 calls
uint256_t balance = web3->EthGetBalance(&address);
```

## Cached public RPC pair (direct nodes):
Fetch a signed best-RPC URL pair for the device `chainId` from universal-node
(`uninode_getRpcPair`), cache it in EEPROM, and talk **directly** to those public HTTPS
nodes (reads and writes). `Web3::exec` **alternates nodes on every call** (round-robin
load-sharing to avoid rate limits); on connection / timeout / HTTP transport errors it
still retries the same call once on the other node. The rotation index is RAM-only
(starts at 0 after boot). With one cached URL, behaviour is unchanged.

**Blocked-node feedback:** connect/TLS failures are logged with the host and mbedtls
`lastError` (e.g. `-29184` / `-80` when a home router SNI-filters the host and replies
with plaintext HTTP). The device only ever tags its **current two cached nodes** in
`uninode_getRpcPair` feedback:
- `{working:[A], blocked:[B]}` when B hits N consecutive connect/TLS failures while A
  succeeds (default N=3, `RPC_PAIR_BLOCK_FAILURE_THRESHOLD`)
- `{blocked:[A,B]}` when both nodes hit that threshold
- `{working:[A,B]}` on weekly / age-based refresh

There is no cumulative blocked list beyond the current pair (uninode returns
rankedList entries below the worst-ranked of the two). Replacement requests are
rate-limited (default once per 30 minutes). On success the signed response is verified
and written to EEPROM (same layout, new timestamp); on uninode failure the current
cache is kept. Call `maintainCachedRpcPair()` from setup and the Wi-Fi check loop
(also throttled inside `Web3::exec`). Default max cache age is 7 days.
See `examples/Cached RPC Pair`.

``` C++
KeyID* keyId = new KeyID(web3);
UniversalNode* uninode = new UniversalNode(web3, keyId);
uninode->syncTime();
web3->useCachedRpcPair(uninode);                // load EEPROM + refresh if needed
web3->maintainCachedRpcPair();                  // call from loop / Wi-Fi reconnect
uint256_t balance = web3->EthGetBalance(&address); // via cached public RPC
```

`setUniversalNode` and `useCachedRpcPair` are alternatives: proxy every read through uninode,
or fetch endpoints once and use public nodes directly.

## Query ERC20 Balance:
``` C++
string address = string("0x007bee82bdd9e866b2bd114780a47f2261c684e3");
Contract contract(web3, "0x20fe562d797a42dcb3399062ae9546cd06f63280"); //contract is on Ropsten

//Obtain decimals to correctly display ERC20 balance (if you already know this you can skip this step)
string param = contract.SetupContractData("decimals()", &address);
string result = contract.ViewCall(&param);
int decimals = web3->getInt(&result);

//Fetch the balance in base units
param = contract.SetupContractData("balanceOf(address)", &address);
result = contract.ViewCall(&param);

uint256_t baseBalance = web3->getUint256(&result);
string balanceStr = Util::ConvertWeiToEthString(&baseBalance, decimals); //convert balance to double style using decimal places
```

## Send ERC20 Token:
``` C++
string contractAddr = "0x20fe562d797a42dcb3399062ae9546cd06f63280";
Contract contract(web3, contractAddr.c_str());
contract.SetPrivateKey(<Your private key>);

//Get contract name
string param = contract.SetupContractData("name()", &addr);
string result = contract.ViewCall(&param);
string interpreted = Util::InterpretStringResult(web3->getString(&result).c_str());
Serial.println(interpreted.c_str());

//Get Contract decimals
param = contract.SetupContractData("decimals()", &addr);
result = contract.ViewCall(&param);
int decimals = web3->getInt(&result);
Serial.println(decimals);

unsigned long long gasPriceVal = 22000000000ULL;
uint32_t  gasLimitVal = 4300000;

//amount of erc20 token to send, note we use decimal value obtained earlier
uint256_t weiValue = Util::ConvertToWei(0.1, decimals);

//get nonce
uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&addr);
string toAddress = "0x007bee82bdd9e866b2bd114780a47f2261c684e3";
string valueStr = "0x00";

//Setup contract function call
string p = contract.SetupContractData("transfer(address,uint256)", &toAddress, &weiValue); //ERC20 function plus params

//push transaction to ethereum
result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &contractAddr, &valueStr, &p);
string transactionHash = web3->getString(&result);
```

## EEPROM Layout and Size Management

**CRITICAL:** Web3E components (KeyID, WifiStore, RpcPairStore) share EEPROM/NVS storage and MUST use the same size.

### The Truncation Bug

On arduino-esp32 3.x, `EEPROM.begin(size)` uses NVS (Non-Volatile Storage) as a blob. **If you call `EEPROM.begin()` with a smaller size than previously stored, it TRUNCATES the blob**, destroying data at higher offsets.

**Example of the bug:**
```cpp
// Boot 1: WifiStore writes at offset 33+
EEPROM.begin(2048);  // Allocates 2048 bytes
// ... write WifiStore data at offset 33-827

// Boot 2: KeyID runs first
EEPROM.begin(33);    // ❌ TRUNCATES blob to 33 bytes!
// WifiStore data at offset 33+ is now GONE
```

### The Fix

All Web3E components now use **`WEB3E_EEPROM_SIZE`** (default 2048 bytes):

```cpp
#include "Web3EEeprom.h"

// KeyID, WifiStore, and RpcPairStore all use WEB3E_EEPROM_SIZE
EEPROM.begin(WEB3E_EEPROM_SIZE);  // ✓ Always 2048 bytes
```

### EEPROM Layout

```
Offset 0-32:     KeyID (1 byte marker + 32 bytes private key)
Offset 33-827:   WifiStore reserved (magic, version, networks, terminator, CRC32)
                 WIFI_MAX_STORAGE_SIZE = 795 → ends at 828
Offset 828+:     RpcPairStore
                 [0..3]   uint32 timestamp (unix secs, little-endian)
                 [4..7]   uint32 chainId   (forces refresh on chain change)
                 [8..]    length-prefixed HTTPS URLs (1 byte len + bytes),
                          repeated (1 or 2), terminated by 0 length byte
Total:           2048 bytes (WEB3E_EEPROM_SIZE)
```

RpcPairStore never writes KeyID or WifiStore bytes, never full-erases flash, and
commits EEPROM only when the cached pair contents change. Blank / 0xFF / corrupt
data is treated as empty.

### Overriding EEPROM Size

If you need a custom size (e.g., for additional data), define it before including Web3E headers:

```cpp
// In platformio.ini
build_flags = -DWEB3E_EEPROM_SIZE=2048

// Or in your sketch
#define WEB3E_EEPROM_SIZE 2048
#include <Web3.h>
```

Minimum: 1349 bytes (KeyID + WifiStore max + RpcPairStore max; enforced at compile time).

**Note:** Changing `WEB3E_EEPROM_SIZE` on an already-provisioned device with a smaller size will truncate existing data. Always use the same or larger size. Increasing from the previous default of 1024 to 2048 is safe (NVS expands).

---

## TCP Bridge Protocol

The TCP Bridge provides a secure connection between IoT devices and the SmartLayerProxy server for TokenScript integration.

### Wire Protocol Opcodes

The protocol uses single-byte opcodes to identify message types:

**Client → Server (Device sends):**
- `0x01`: Refresh request - Device requests token list `[opcode][20-byte address]`
- `0x06`: Keep-alive ping - Connection maintenance `[opcode]`
- `0x07`: Challenge response (legacy) - Authentication without firmware version `[opcode][20-byte address][65-byte signature]`
- `0x08`: API response - Result from API call `[opcode][response string]`
- `0x09`: Challenge response with firmware version - Authentication including firmware version (see below)
- `0x0A`: WiFi diagnostics - Connection diagnostics after successful login (see below)

**Server → Client (Device receives):**
- `0x02`: Challenge - Server sends authentication challenge `[opcode][16-byte random challenge]`
- `0x04`: API call - Server requests device to execute an API
- `0x06`: Keep-alive - Server sends keep-alive

### Challenge Response with Firmware Version (Opcode 0x09)

When a firmware version is set via `setFirmwareVersion()`, the device authenticates using opcode `0x09`:

**Wire Format:**
```
[1 byte: 0x09][20 bytes: address][65 bytes: signature][1 byte: versionLen][versionLen bytes: version]
```

**Fields:**
- **Opcode** (1 byte): `0x09`
- **Address** (20 bytes): Device's Ethereum address
- **Signature** (65 bytes): ECDSA signature (r, s, v format)
- **Version Length** (1 byte): Length of firmware version string (0-32)
- **Version** (variable): ASCII firmware version string

**Signature Preimage:**

The signature is computed over `keccak256(challenge || versionLen || version)` where:
- `challenge`: 16-byte random challenge from server (opcode 0x02)
- `versionLen`: 1-byte length of version string
- `version`: ASCII firmware version string

**Example:**

For firmware version "v1.2.3" (6 bytes):
1. Server sends: `0x02` + 16-byte challenge
2. Device computes: `preimage = challenge || 0x06 || "v1.2.3"`
3. Device signs: `signature = ecdsa_sign(keccak256(preimage), privateKey)`
4. Device sends: `0x09 || address(20) || signature(65) || 0x06 || "v1.2.3"`

**Server Verification (SmartLayerProxy):**

The server verifies the signature by:
1. Extract `address`, `signature`, `versionLen`, and `version` from the packet
2. Reconstruct preimage: `challenge || versionLen || version`
3. Compute hash: `keccak256(preimage)`
4. Recover signer: `ecrecover(hash, signature)`
5. Verify: `recovered_address == address`

### Setting Firmware Version

**At compile time:**
```cpp
// In platformio.ini
build_flags =
    -DFW_VERSION=\"v1.2.3\"
```

**At runtime:**
```cpp
TcpBridge *tcpConnection = new TcpBridge();
tcpConnection->setFirmwareVersion("v1.2.3");
tcpConnection->setKey(keyID, web3);
tcpConnection->startConnection();
```

**Clearing firmware version (revert to opcode 0x07):**
```cpp
tcpConnection->setFirmwareVersion(nullptr);
```

### Backward Compatibility

**Older firmware must NOT break. Servers MUST accept both opcodes.**

- **Legacy devices** (no firmware version set, or `setFirmwareVersion(nullptr)`) use opcode `0x07`
- **New devices** (with firmware version) use opcode `0x09`
- **Old servers** accepting only opcode `0x07` continue working with legacy devices
- **New servers** (SmartLayerProxy) MUST accept BOTH opcodes `0x07` (no version) and `0x09` (with version)
- Opcode `0x07` behavior is byte-for-byte identical to the original implementation
- The firmware version in opcode `0x09` is authenticated by including it in the signed challenge preimage

## WiFi Connection Diagnostics (Opcode 0x0A)

WiFi connection is the primary failure point for IoT devices. Web3E includes comprehensive diagnostics tracking to identify connectivity issues.

### Features

**Watchdog Timer:**
- Configurable timeout for WiFi connection (default 30 seconds per network)
- Cycles through all configured networks up to 3 times
- Automatic reboot if no network connects after all attempts
- Prevents infinite hangs during startup

**Diagnostics Tracking:**
- Reset reason (power-on, software restart, watchdog, brownout, etc.)
- Boot count since last successful TCP bridge login
- WiFi connection attempts before success
- Time to establish WiFi connection (milliseconds)
- Network index that succeeded (0-based)
- Last failure stage (association, authentication, DHCP, DNS, TLS, proxy)
- Last WiFi disconnect reason code

**Storage:**
- Uses ESP32 NVS (Non-Volatile Storage) for persistence across reboots
- Survives soft reboots and retains data through power cycles
- Does NOT overlap with KeyID EEPROM storage (bytes 0-32)
- Automatically resets counters after successful TCP bridge login

### Wire Format (Opcode 0x0A)

Sent automatically after successful TCP bridge login (challenge response accepted), once per boot cycle.

```
[1 byte: 0x0A][1 byte: resetReason][2 bytes: bootCount][2 bytes: wifiAttempts]
[4 bytes: connectTimeMs][1 byte: networkIndex][1 byte: lastFailureStage]
[1 byte: lastDisconnectReason][1 byte: reserved]
```

**Total packet size: 14 bytes**

**Field Details:**

| Offset | Length | Type | Field | Description |
|--------|--------|------|-------|-------------|
| 0 | 1 | uint8 | opcode | `0x0A` (diagnostics) |
| 1 | 1 | uint8 | resetReason | ESP32 reset reason code (see below) |
| 2 | 2 | uint16 | bootCount | Number of boots since last successful login (big-endian) |
| 4 | 2 | uint16 | wifiAttempts | WiFi connection attempts before success (big-endian) |
| 6 | 4 | uint32 | connectTimeMs | Milliseconds to establish WiFi connection (big-endian) |
| 10 | 1 | uint8 | networkIndex | Index of network that succeeded (0-based) |
| 11 | 1 | uint8 | lastFailureStage | Last failure stage code (see below) |
| 12 | 1 | uint8 | lastDisconnectReason | WiFi disconnect reason code (see below) |
| 13 | 1 | uint8 | reserved | Reserved for future use (set to 0) |

**Reset Reason Codes (`esp_reset_reason_t` from ESP-IDF):**
- `0`: ESP_RST_UNKNOWN - Unknown reset reason
- `1`: ESP_RST_POWERON - Power-on reset
- `2`: ESP_RST_EXT - External pin reset
- `3`: ESP_RST_SW - Software reset via esp_restart
- `4`: ESP_RST_PANIC - Software panic reset
- `5`: ESP_RST_INT_WDT - Interrupt watchdog reset
- `6`: ESP_RST_TASK_WDT - Task watchdog reset
- `7`: ESP_RST_WDT - Other watchdog reset
- `8`: ESP_RST_DEEPSLEEP - Deep sleep reset
- `9`: ESP_RST_BROWNOUT - Brownout reset
- `10`: ESP_RST_SDIO - SDIO reset

**Failure Stage Codes:**
- `0x00`: None (no failure)
- `0x01`: Association failed (SSID not found)
- `0x02`: Authentication failed (wrong password)
- `0x03`: DHCP failed (no IP address)
- `0x04`: DNS resolution failed
- `0x05`: TLS handshake failed
- `0x06`: Proxy connection failed

**WiFi Disconnect Reason Codes:**
ESP32 `wifi_err_reason_t` enum values (200+ range):
- `200`: WIFI_REASON_BEACON_TIMEOUT - Beacon timeout
- `201`: WIFI_REASON_NO_AP_FOUND - AP not found
- `202`: WIFI_REASON_AUTH_FAIL - Authentication failed
- `203`: WIFI_REASON_ASSOC_FAIL - Association failed
- `204`: WIFI_REASON_HANDSHAKE_TIMEOUT - 4-way handshake timeout
- And others per ESP32 WiFi API (see `esp_wifi_types.h`)

All codes fit in uint8 field (0-255 range).

### Usage

**Enable diagnostics in your sketch:**

```cpp
#include <TcpBridge.h>
#include <WiFiConnection.h>

WiFiConnection wifiConnection;
TcpBridge *tcpConnection;

void setup() {
    // Connect to WiFi
    WiFiCredentials networks[] = {
        {"Network1", "password1"},
        {"Network2", "password2"},
        {nullptr, nullptr}  // Terminator
    };
    wifiConnection.setupWifi(networks);
    
    // Optional: Set connection timeout (default 30000ms)
    wifiConnection.setConnectionTimeout(45000); // 45 seconds
    
    // Setup TCP bridge with diagnostics
    tcpConnection = new TcpBridge();
    tcpConnection->setFirmwareVersion("v1.0.0");
    tcpConnection->setWiFiDiagnostics(wifiConnection.getDiagnostics());
    tcpConnection->setKey(keyID, web3);
    tcpConnection->startConnection();
}
```

**Diagnostics are sent automatically:**
1. Device boots and records boot count
2. WiFi connection established with timing/attempts tracked
3. TCP bridge connects and authenticates (challenge/response)
4. After successful authentication, diagnostics sent once (opcode `0x0A`)
5. Diagnostics counters reset after successful send

**Server Implementation (SmartLayerProxy):**

Old servers safely ignore unknown opcode `0x0A` - the TCP protocol is designed so unknown opcodes can be skipped. New servers should parse and store diagnostics:

```typescript
// Parse opcode 0x0A diagnostics packet (14 bytes)
const opcode = buffer[0]; // 0x0A
const resetReason = buffer[1];
const bootCount = (buffer[2] << 8) | buffer[3];
const wifiAttempts = (buffer[4] << 8) | buffer[5];
const connectTimeMs = (buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];
const networkIndex = buffer[10];
const lastFailureStage = buffer[11];
const lastDisconnectReason = buffer[12];
const reserved = buffer[13];

// Store diagnostics associated with device address
console.log(`Device diagnostics: ${bootCount} boots, ${wifiAttempts} attempts, ${connectTimeMs}ms connect time`);
```

### Backward Compatibility

- Old firmware without diagnostics: No opcode `0x0A` sent (fully compatible)
- New firmware with diagnostics: Sends opcode `0x0A` after login
- Old servers: Unknown opcode safely ignored (TCP protocol is opcode-extensible)
- New servers: Parse and store diagnostics for monitoring

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
