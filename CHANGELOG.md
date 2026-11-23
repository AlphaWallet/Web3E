# Changelog

All notable changes to Web3E will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### 🔴 BREAKING CHANGES

- **Legacy RPC node selection removed.** `src/nodes.h`, `src/Certificates.h`,
  Infura opt-in (`USING_INFURA` / `INFURA_API_KEY` / `Web3(chainId, infura_key)`),
  and automatic `selectHost()` are gone. `Web3(chainId)` no longer picks a public
  RPC. Call `useCachedRpcPair(uninode)` (preferred; any chainlist.org chain uninode
  serves) or `setRpcUrl("https://…")` before JSON-RPC. EEPROM KeyID (0–32) and
  WifiStore layout are unchanged.

- **Default proxy server changed** from `scriptproxy.smarttokenlabs.com:8003` to `scriptproxy.percolate.one:8083` (TCP) / `:8083` (UDP)
  - **Impact:** Devices using the default proxy (no custom configuration) will fail to connect with DNS errors
  - **Required action:** Update existing deployments to either:
    1. Use the new default (recommended): No code changes needed, just update Web3E
    2. Configure a custom proxy: Use `setProxy()` (TCP) or `setupConnection()` (UDP) at runtime, or set `WEB3E_PROXY_HOST`/`WEB3E_PROXY_PORT` build flags
  - The old `scriptproxy.smarttokenlabs.com` domain no longer resolves

### Changed

- **Cached RPC pair blocked/working feedback:** `uninode_getRpcPair` params may be
  `[chainId]` or `[chainId,{"blocked":[...],"working":[...]}]` (≤8 URLs each; firmware
  only tags the current cached pair). Cases: `{working:[A],blocked:[B]}` when one
  node is TLS-blocked and the other works; `{blocked:[A,B]}` when both hit the
  failure threshold; `{working:[A,B]}` on weekly refresh. No cumulative blocked
  list. Replacement is rate-limited (30 min); signature verified; EEPROM updated
  on success. Connect failures always log host + mbedtls `lastError` (SNI/filter
  intercept detection for `-29184` / `-80`). Round-robin + one-shot retry unchanged.
  EEPROM layout unchanged (never touches KeyID 0–32 or WifiStore).

- **Cached RPC pair load-sharing:** `Web3::exec` alternates between the two cached
  URLs on every request (round-robin, RAM index only) to avoid public-RPC rate
  limits. Transport failure still retries the same call once on the other node.
  Single-URL cache behaviour unchanged.

### Added

- **Cached public RPC pair from universal-node**
  - `Web3::useCachedRpcPair(uninode)` fetches signed best RPC URLs via `uninode_getRpcPair`
  - Wire matches JamesSmartCell/universal-node PR #4: params[0]==envelope chainId; signed v1 success with `result:{chainId,timestamp,rpcs[]}` and `blockNumber:"0"`; unsigned HTTP `{error:{code:RPC_VALIDATION,message}}` on failure (keep cache)
  - Optional blocked/working feedback object on getRpcPair (server built in parallel)
  - EEPROM `RpcPairStore` at offset 828 (after WifiStore reserved region): timestamp + chainId + length-prefixed HTTPS URLs
  - chainId stored in cache (forces refresh on chain change — small addition to the pair cache layout)
  - Refresh at startup / Wi-Fi check loop / ~24h; max age default 7 days; keep cache on refresh failure
  - Per-request round-robin load-sharing; transport failure retries once on the other node
  - TLS via `setInsecure` on HTTPS RPC (no per-node certs)
  - Example: `examples/Cached RPC Pair` (`huge_app` partition)
  - Default `WEB3E_EEPROM_SIZE` raised to 2048 to fit RpcPairStore (min 1349)
  - `Web3::setRpcUrl(httpsUrl)` — optional explicit single-URL override (disables cached-pair mode)

- **universal-node signed read-only RPC client**
  - `UniversalNode` / `UniversalNodeProtocol` — device client for `https://uninode.percolate.one`
  - EIP-191 `personal_sign` requests with device `KeyID` (EEPROM bytes 0–32 unchanged)
  - Response verification against pinned node signer (default `0xF09b881B849B50D9A3d96b7Aed50f34b5F585A30`)
  - NTP time sync required (±60s window); fails clearly if clock not synced
  - `Web3::setUniversalNode()` routes `EthGetBalance`, `EthViewCall` / `EthCall`, `EthBlockNumber`, `EthChainId`, `EthGetTransactionReceipt`
  - Native host tests: `tests/test_universal_node` (canonical JSON, request messages, response verify)
  - Example sketch: `examples/Universal Node RPC`

- **TCP Bridge proxy configuration**
  - `TcpBridge::setProxy(host, port)` - Runtime proxy configuration (call before `startConnection()`)
  - `WEB3E_PROXY_HOST` / `WEB3E_PROXY_PORT` - Compile-time overrides (also honors `PROXY_HOST`/`PROXY_PORT`)
  - Prints configured proxy on initialization: `TCP Bridge: scriptproxy.percolate.one:8083`

- **UDP Bridge proxy configuration**
  - Default host now configurable via `WEB3E_PROXY_HOST` / `WEB3E_PROXY_PORT` or `PROXY_HOST`/`PROXY_PORT` build flags
  - `setupConnection()` now correctly uses the configured `serverName` instead of the old hardcoded `hostName`
  - Prints configured proxy on initialization: `UDP Bridge: scriptproxy.percolate.one:8083`

- **Firmware version reporting (opcode 0x09)**
  - `TcpBridge::setFirmwareVersion(const char* version)` - Set firmware version (max 32 bytes ASCII)
  - Optional compile-time default via `-DFW_VERSION="v1.2.3"` build flag
  - Authenticated via signature over `keccak256(challenge || versionLen || version)`
  - Wire format: `[0x09][20-byte address][65-byte signature][1-byte versionLen][version bytes]`
  - Backward compatible - legacy opcode 0x07 (no version) still supported

- **WiFi connection diagnostics (opcode 0x0A)**
  - Comprehensive diagnostics tracking: reset reason, boot count, WiFi attempts, connect time, failure stage, disconnect reason
  - WiFi watchdog: configurable timeout (default 30s), cycles through networks up to 3 times, auto-reboot on failure
  - `WiFiConnection::setConnectionTimeout(ms)` - Configure WiFi connection timeout
  - `WiFiConnection::getDiagnostics()` - Get diagnostics tracker
  - `TcpBridge::setWiFiDiagnostics()` - Enable automatic diagnostics reporting
  - Diagnostics sent automatically after successful TCP bridge login (once per boot)
  - Stored in ESP32 NVS (survives reboots)
  - Wire format: 14-byte packet with boot count, WiFi attempts, connect time, failure codes
  - Backward compatible - old servers ignore unknown opcode

- **WifiStore EEPROM reader**
  - Read WiFi credentials from EEPROM in Web3E-Setup-Firmware format
  - EEPROM layout: offset 33+, magic "WIFI", version 1, networks with CRC32 validation
  - `WifiStore::begin()` / `isValid()` / `readNetworks()` / `getNetworkCount()`
  - Compatible with Web3E-Setup-Firmware provisioning tool
  - Max 8 networks, SSID ≤32 bytes, password ≤64 bytes

- **Shared EEPROM size management**
  - `WEB3E_EEPROM_SIZE` (default 1024 bytes) - Shared constant for KeyID and WifiStore
  - Prevents NVS truncation bug on arduino-esp32 3.x
  - Overridable via build flag: `-DWEB3E_EEPROM_SIZE=2048`
  - Layout: KeyID at 0-32, WifiStore at 33+, total 1024 bytes
  - All components now log EEPROM size on initialization

- **Hardware E2E test suite**
  - Comprehensive test firmware in `test/hardware/proxy_e2e/`
  - Exercises full TCP bridge protocol: opcodes 0x09, 0x0A, 0x06, 0x04/0x08
  - TypeScript test runner with serial monitoring and telemetry verification
  - Machine-parseable `E2E:` protocol at 115200 baud
  - Automated firmware version via git SHA: `e2e-<sha>`
  - SmartLayerProxy telemetry API integration

### Fixed

- **🔥 CRITICAL: EEPROM truncation bug** (discovered on real hardware, arduino-esp32 3.x)
  - `EEPROM.begin()` with smaller size than previously stored TRUNCATES the NVS blob
  - KeyID calling `EEPROM.begin(33)` was destroying WifiStore data at offset 33+
  - Observed as WifiStore CRC validation failures after reboot
  - **Solution:** All components now use shared `WEB3E_EEPROM_SIZE` (1024 bytes)
  - Existing provisioned devices preserved (layout unchanged)

- **ESP-IDF reset reason codes corrected** in documentation
  - Now shows correct values: 0=UNKNOWN, 1=POWERON, 2=EXT, 3=SW, etc. (0-10)
  - Previously showed incorrect values

- **WiFi disconnect reason codes clarified** in documentation
  - ESP32 uses 200+ range: 200=BEACON_TIMEOUT, 201=NO_AP_FOUND, 202=AUTH_FAIL, etc.
  - All fit in uint8 field (0-255)

### Changed

- **TCP Bridge default:** `scriptproxy.percolate.one:8083` (was: `scriptproxy.smarttokenlabs.com:8003`)
- **UDP Bridge default:** `scriptproxy.percolate.one:8083` (was: `scriptproxy.smarttokenlabs.com:8003`)
- SmartLayerProxy GitHub repo link updated (now TypeScript implementation)

### Deprecated

- The old `scriptproxy.smarttokenlabs.com` domain is no longer used and does not resolve

---

## [1.45] - 2026-11-16

### Fixed
- Fix case-sensitive include issues for Linux/PlatformIO builds
- Fix `byte` type ambiguity with modern C++ compilers
- Add CI testing to verify library packaging
- Enable consumption via git URL with version tags

### Added
- GitHub Actions release workflow
- Automated publishing to PlatformIO Registry

---

## Earlier Versions

See README.md "What's New" section for changes in v1.44 and earlier.
