# Wallet Bridge Testing - Future Work

## Overview
The Wallet Bridge examples (1, 2, and 3) have been updated to use WiFiConnection, but comprehensive automated tests have not been implemented yet due to their complexity.

## Why Wallet Bridge Tests Are Complex

Wallet Bridge tests require:
1. A running TcpBridge server connection
2. External wallet interaction (AlphaWallet or similar)
3. Challenge-response protocol testing
4. Signature verification round-trips
5. For Bridge 3: On-chain token ownership verification

These tests are better suited for manual testing or integration tests with a full setup rather than automated hardware tests.

## What Each Bridge Example Does

### Wallet Bridge 1 (Basic Challenge-Response)
- Device creates a random challenge
- Wallet requests challenge via TCP bridge
- Wallet signs challenge with user's private key
- Device verifies signature and recovers signer address
- If signature is valid, device blinks LED

### Wallet Bridge 2 (With Allowlist)
- Same as Bridge 1, but adds account verification
- Device maintains a list of allowed Ethereum addresses
- After signature verification, checks if recovered address is in allowlist
- Only allowed accounts can operate the device

### Wallet Bridge 3 (With Token Gating)
- Same as Bridge 1/2, but verifies token ownership
- After signature verification, queries blockchain to check if signer owns specific ERC721/875 token
- Only token holders can operate the device
- Demonstrates token-gated IoT access control

## Manual Testing Procedure

To manually test Wallet Bridge functionality:

1. **Setup Device**:
   - Flash Wallet Bridge example to ESP32
   - Note the device Ethereum address from serial output
   - Ensure device connects to TCP bridge server

2. **Setup Wallet**:
   - Install AlphaWallet (iOS/Android)
   - Create test account and fund with testnet ETH
   - For Bridge 2: Add wallet address to `allowedAccounts` array in code
   - For Bridge 3: Deploy ERC721 token, transfer to wallet, update TOKEN_CONTRACT

3. **Test Flow**:
   - Open wallet and navigate to token/action
   - Wallet fetches challenge from device
   - Sign challenge in wallet
   - Observe device LED blink on successful verification

## Future Automated Test Ideas

If automated testing is needed:

1. **Mock TcpBridge Test**:
   - Create mock TCP server
   - Simulate wallet challenge requests
   - Test signature verification logic in isolation

2. **Unit Tests**:
   - Test challenge generation (randomness, format)
   - Test signature verification with known keypairs
   - Test address allowlist checking (Bridge 2)
   - Test token balance queries (Bridge 3)

3. **Integration Test**:
   - Spawn actual TcpBridge server
   - Use Web3 library to simulate wallet signing
   - Full end-to-end flow without manual interaction
   - Requires test infrastructure setup

## Current Status

**Completed**:
- All Wallet Bridge examples updated to use WiFiConnection
- Examples compile successfully
- Network configuration simplified
- Code consistency with other examples

**Not Implemented**:
- Automated hardware tests for bridge protocol
- TcpBridge server mocking
- Challenge-response round-trip tests

**Recommendation**: Manual testing is currently the best approach for Wallet Bridge functionality verification.
