# AploEmbed Scouting Report

**Date:** 2026-05-20  
**Task:** t_33ece8ee  
**Objective:** Map Web3E leftovers, WebMiner behavior, AploNode RPC methods for AploCoin-only refactoring

---

## Executive Summary

AploEmbed is a fork of Web3E (Ethereum library for Arduino/ESP32). Current state:
- **Multi-chain infrastructure present** — needs removal for AploCoin-only focus
- **WebMiner uses standard Ethereum JSON-RPC** — balance, nonce, sendRawTransaction, contract calls
- **AploNode is go-ethereum fork** — standard eth_* RPC methods available
- **AploCoin Chain ID: 28282** (found in aplonode/params/config.go)
- **Default RPC endpoints:** pub1.aplocoin.com, pub2.aplocoin.com (from WebMiner)

---

## 1. AploEmbed: Web3E Multi-Chain Leftovers

### 1.1 Files with Multi-Chain Logic

| File | Multi-Chain Elements | Action Required |
|------|---------------------|-----------------|
| `src/chainIds.h` | 40+ chain ID constants (MAINNET_ID, POLYGON_ID, SEPOLIA_ID, etc.) | **Remove all except APLO_ID** |
| `src/nodes.h` | `getNode(chainId)` switch with 30+ cases, Infura URLs | **Replace with single Aplo RPC provider logic** |
| `src/Web3.h` | Constructor takes `long long chainId`, stores it | **Simplify to fixed APLO_CHAIN_ID** |
| `src/Web3.cpp` | `selectHost()` parses node URLs, Infura key logic | **Hardcode pub1/pub2.aplocoin.com with fallback** |
| `src/Certificates.h` | TLS certs for multiple chains | **Keep only Aplo cert (if needed)** |
| `README.md` | Examples reference KOVAN_ID, SEPOLIA_ID, MAINNET_ID | **Update to APLO_ID** |

### 1.2 Constructor Pattern (Current)

```cpp
Web3::Web3(long long networkId) {
    chainId = networkId;
    selectHost();  // calls getNode(chainId) from nodes.h
}
```

**Recommendation:** Replace with:
```cpp
Web3::Web3(const char* rpcUrl = "pub1.aplocoin.com", const char* fallbackUrl = "pub2.aplocoin.com") {
    chainId = 28282;  // Fixed for AploCoin
    primaryRpc = rpcUrl;
    fallbackRpc = fallbackUrl;
}
```

### 1.3 Naming & Documentation

- **Library name:** Still "Web3E" in README, library.json, library.properties
- **Action:** Rename to "AploEmbed" or "AploSDK" throughout
- **Examples:** 7 examples reference Ethereum testnets (Kovan, Sepolia, Rinkeby)
- **Action:** Update to AploCoin addresses/contracts

---

## 2. WebMiner: RPC Usage Patterns

**Source:** `/home/hermes/projects/aplosdk/webminer/src/components/webMiner.tsx`

### 2.1 RPC Endpoints Used

```typescript
const RPC_URL = "https://pub1.aplocoin.com";
const CONTRACT_ADDRESS = "0x0000000000000000000000000000000000001234";  // Mining contract
const APLO_STAKING_ADDRESS = "0x0000000000000000000000000000000000001235";  // Staking contract
```

### 2.2 RPC Methods Called

| Method | Purpose | Payload Example |
|--------|---------|-----------------|
| `eth_getBalance` | Query APLO balance | `["0xAddress", "latest"]` |
| `eth_getTransactionCount` | Get nonce for signing | `["0xAddress", "pending"]` |
| `eth_sendRawTransaction` | Submit signed tx | `["0x...signedRlp"]` |
| `eth_call` | Read-only contract call | `{to: "0x1234", data: "0x..."}` |
| `eth_getBlockNumber` | Current block height | `[]` |
| `eth_gasPrice` | Current gas price | `[]` |

### 2.3 Contract Interactions

**Mining Contract (0x1234):**
- `miner_params(address)` → returns `(last_block, current_difficulty, total_mined, prev_hash)`
- `mine(bytes32 nonce)` → submit mined block

**Staking Contract (0x1235):**
- `getStake(address)` → returns staked amount in Wei
- `getMultiplier(address)` → returns multiplier (scaled by 10)
- `stake(uint256 amount)` → stake APLO
- `unstake()` → withdraw stake

### 2.4 Transaction Signing Flow

```typescript
1. Get nonce: eth_getTransactionCount(address, "pending")
2. Estimate gas: contract.methods.mine(nonce).estimateGas()
3. Get gas price: eth_gasPrice()
4. Build tx: { from, to, data, gas, gasPrice, nonce }
5. Sign locally: web3.eth.accounts.signTransaction(tx, privateKey)
6. Send: eth_sendRawTransaction(signedTx.rawTransaction)
```

**Key insight:** WebMiner does **client-side signing** with private key. AploEmbed must support this pattern.

---

## 3. AploNode: RPC Implementation

**Source:** `/home/hermes/projects/aplosdk/aplonode/internal/ethapi/api.go`

### 3.1 Standard Ethereum JSON-RPC Methods

AploNode is a go-ethereum fork, so it implements standard eth_* namespace:

| Method | Function | Line |
|--------|----------|------|
| `eth_getBalance` | `BlockChainAPI.GetBalance(address, blockNrOrHash)` | 639 |
| `eth_getTransactionCount` | `TransactionAPI.GetTransactionCount(address, blockNrOrHash)` | 1545 |
| `eth_sendRawTransaction` | `TransactionAPI.SendRawTransaction(input)` | 1773 |
| `eth_call` | `BlockChainAPI.Call(args, blockNrOrHash, overrides)` | 1031 |
| `eth_gasPrice` | `EthereumAPI.GasPrice(ctx)` | 62 |
| `eth_blockNumber` | Standard go-ethereum | - |

### 3.2 Mining/Staking (Custom?)

WebMiner calls `miner_params(address)` and `mine(bytes32)` — these are **smart contract methods**, not RPC endpoints. AploNode serves them via `eth_call` (read) and `eth_sendRawTransaction` (write).

### 3.3 Chain ID

**Location:** `aplonode/params/config.go:44`

```go
MainnetChainConfig = &ChainConfig{
    ChainID: big.NewInt(28282),
    ChainID_ALT: big.NewInt(28282),
    // ...
}
```

**Confirmed:** AploCoin Chain ID = **28282**

---

## 4. Implementation Handoff for Coder

### 4.1 Files to Edit

| File | Changes |
|------|---------|
| `src/chainIds.h` | Remove all constants except `#define APLO_ID 28282` |
| `src/nodes.h` | Replace `getNode(chainId)` with `getAploNode()` returning "pub1.aplocoin.com" |
| `src/Web3.h` | Change constructor: `Web3(const char* rpcUrl = nullptr, const char* fallbackUrl = nullptr)` |
| `src/Web3.cpp` | Hardcode chainId = 28282, simplify `selectHost()` to use Aplo RPCs |
| `src/Certificates.h` | Keep only Aplo cert (or setInsecure if pub1/pub2 use valid certs) |
| `README.md` | Replace all Ethereum references with AploCoin |
| `library.json` | Rename to "AploEmbed" |
| `library.properties` | Rename to "AploEmbed" |
| `keywords.txt` | Update if needed |
| `examples/*` | Update all 7 examples to use APLO_ID, Aplo addresses |

### 4.2 Symbols/APIs to Remove

**From chainIds.h:**
- All `*_ID` constants except APLO_ID

**From nodes.h:**
- All `*_RPC_URL` constants except Aplo
- `getNode(chainId)` switch logic

**From Web3.cpp:**
- Infura key logic (lines 19-36, 351-354)
- Multi-chain selectHost() logic

### 4.3 New API Design

**Constructor:**
```cpp
// Default: use pub1 with pub2 fallback
Web3();

// Custom RPC
Web3(const char* primaryRpc);

// Custom RPC with fallback
Web3(const char* primaryRpc, const char* fallbackRpc);
```

**Internal:**
```cpp
private:
    const char* primaryRpc = "pub1.aplocoin.com";
    const char* fallbackRpc = "pub2.aplocoin.com";
    long long chainId = 28282;  // Fixed
```

### 4.4 RPC Method Table (Required for AploCoin)

| Method | Current Support | Notes |
|--------|----------------|-------|
| `eth_getBalance` | ✅ `Web3::EthGetBalance(address)` | Line 136 |
| `eth_getTransactionCount` | ✅ `Web3::EthGetTransactionCount(address)` | Line 156 |
| `eth_sendRawTransaction` | ✅ `Web3::EthSendSignedTransaction(data, len)` | Line 174 |
| `eth_call` | ✅ `Web3::EthCall(...)` | Line 164 |
| `eth_gasPrice` | ✅ `Web3::EthGasPrice()` | Line 116 |
| `eth_blockNumber` | ✅ `Web3::EthBlockNumber()` | Line 128 |
| Contract calls | ✅ Via `Contract` class | `Contract.cpp` |

**All required methods already implemented.** No new RPC methods needed.

### 4.5 Example Call Patterns (from WebMiner)

**Get Balance:**
```cpp
string address = "0x...";
uint256_t balance = web3->EthGetBalance(&address);
string balanceEth = Util::ConvertWeiToEthString(&balance, 18);
```

**Send Transaction:**
```cpp
Contract contract(web3, "0x1234");  // Mining contract
contract.SetPrivateKey(PRIVATE_KEY);
uint32_t nonce = web3->EthGetTransactionCount(&address);
uint256_t gasPrice = web3->EthGasPrice();
string txHash = contract.SendTransaction(nonce, gasPrice, 90000, &to, &value, &data);
```

**Check Staking:**
```cpp
Contract staking(web3, "0x1235");
string param = staking.SetupContractData("getStake(address)", &myAddress);
string result = staking.ViewCall(&param);
uint256_t staked = web3->getUint256(&result);
```

---

## 5. Risks & Unknowns

### 5.1 TLS Certificates

- **Current:** Web3E has hardcoded certs for Infura, Polygon, etc. in `Certificates.h`
- **Risk:** pub1/pub2.aplocoin.com may use different certs or self-signed
- **Mitigation:** Test connection, use `client->setInsecure()` if needed (acceptable for embedded)

### 5.2 RPC Endpoint Stability

- **Unknown:** Are pub1/pub2 load-balanced? Do they support HTTPS?
- **Recommendation:** Implement retry logic with fallback in `Web3::exec()`

### 5.3 Gas Price Strategy

- WebMiner uses `eth_gasPrice()` directly
- **Question:** Does AploCoin have dynamic gas pricing or fixed?
- **Action:** Test and document in examples

### 5.4 Contract Addresses

- Mining: `0x0000000000000000000000000000000000001234`
- Staking: `0x0000000000000000000000000000000000001235`
- **Risk:** These look like placeholders. Verify actual deployed addresses.

### 5.5 Backward Compatibility

- **Breaking change:** Removing multi-chain support breaks existing Web3E users
- **Mitigation:** This is a fork (AploEmbed), not an update to Web3E

---

## 6. Testing Checklist

After refactoring, verify:

- [ ] Connect to pub1.aplocoin.com via HTTPS
- [ ] Query balance of known address
- [ ] Get transaction count (nonce)
- [ ] Sign and send a transaction
- [ ] Call mining contract `miner_params(address)`
- [ ] Call staking contract `getStake(address)`
- [ ] Fallback to pub2 if pub1 fails
- [ ] All 7 examples compile and run
- [ ] Library size reduced (removed unused chain configs)

---

## 7. Recommended Commit Sequence

1. **Add APLO_ID constant** to `chainIds.h`
2. **Update nodes.h** with Aplo RPC logic
3. **Refactor Web3 constructor** to remove chainId param
4. **Remove Infura logic** from Web3.cpp
5. **Update README** with AploCoin branding
6. **Rename library** in library.json/properties
7. **Update examples** (one commit per example)
8. **Remove unused chain constants** (cleanup commit)

Each commit should compile and pass basic smoke tests.

---

## 8. Files Inventory

### Core Library (src/)
- `Web3.h` / `Web3.cpp` — Main RPC client ✏️ **EDIT**
- `Contract.h` / `Contract.cpp` — Smart contract interaction ✅ **KEEP**
- `Crypto.h` / `Crypto.cpp` — ECDSA signing ✅ **KEEP**
- `Util.h` / `Util.cpp` — Wei/Eth conversion ✅ **KEEP**
- `chainIds.h` — Chain ID constants ✏️ **EDIT**
- `nodes.h` — RPC endpoint mapping ✏️ **EDIT**
- `Certificates.h` — TLS certs ✏️ **EDIT**
- `types.h` — Type definitions ✅ **KEEP**
- `KeyID.h` / `KeyID.cpp` — Key management ✅ **KEEP**
- `TcpBridge.h` / `TcpBridge.cpp` — TokenScript bridge ✅ **KEEP**
- `UdpBridge.h` / `UdpBridge.cpp` — TokenScript bridge ✅ **KEEP**
- `ScriptClient.h` / `ScriptClient.cpp` — API client ✅ **KEEP**
- `APIReturn.h` — API response struct ✅ **KEEP**

### Crypto (src/Trezor/)
- All files ✅ **KEEP** (Trezor crypto library, no chain-specific logic)

### Math (src/uint256/)
- All files ✅ **KEEP** (uint256 implementation)

### Examples (examples/)
- 7 example projects ✏️ **UPDATE** (change chain IDs, addresses)

### Metadata
- `README.md` ✏️ **REWRITE**
- `library.json` ✏️ **RENAME**
- `library.properties` ✏️ **RENAME**
- `keywords.txt` ✏️ **UPDATE**
- `LICENSE` ✅ **KEEP**

---

## 9. Downstream Impact

### WebMiner
- **No changes needed** — uses standard JSON-RPC over HTTPS
- Already points to pub1.aplocoin.com

### AploNode
- **No changes needed** — standard go-ethereum RPC
- Chain ID already set to 28282

### Future Integrations
- Any tool using AploEmbed will only support AploCoin
- Multi-chain support removed by design

---

## 10. Next Steps

**For Coder:**
1. Load this report
2. Follow commit sequence in section 7
3. Test each commit against pub1.aplocoin.com
4. Update examples with real Aplo addresses
5. Verify WebMiner still works after changes

**For Reviewer:**
1. Check that no Ethereum chain IDs remain
2. Verify RPC endpoints are Aplo-only
3. Test fallback logic (disconnect pub1, verify pub2 used)
4. Review examples for correctness

**For Tester:**
1. Run all 7 examples on ESP32
2. Verify mining contract interaction
3. Verify staking contract interaction
4. Load test RPC endpoints

---

## Appendix A: Chain ID Source

**File:** `aplonode/params/config.go`  
**Lines:** 43-66

```go
MainnetChainConfig = &ChainConfig{
    ChainID:                 big.NewInt(28282),
    ChainID_ALT:             big.NewInt(28282),
    // ... rest of config
}
```

---

## Appendix B: WebMiner RPC Constants

**File:** `webminer/src/components/webMiner.tsx`  
**Lines:** 29-33

```typescript
const RPC_URL = "https://pub1.aplocoin.com";
const CONTRACT_ADDRESS = "0x0000000000000000000000000000000000001234";
const APLO_STAKING_ADDRESS = "0x0000000000000000000000000000000000001235";
const MIN_STAKE_APLO = "1000";
```

---

## Appendix C: Current Web3E Multi-Chain Support

**Chains in chainIds.h (43 total):**
- Ethereum Mainnet, Ropsten, Rinkeby, Kovan, Goerli, Sepolia
- Polygon, Mumbai
- Binance Smart Chain, Binance Testnet
- Avalanche, Fuji
- Fantom, Fantom Testnet
- Optimism, Arbitrum
- Klaytn, IoTeX, Aurora, Milkomeda, Palm, Cronos, Heco, xDai, POA, Sokol, Artis

**All to be removed except AploCoin (28282).**

---

**End of Report**
