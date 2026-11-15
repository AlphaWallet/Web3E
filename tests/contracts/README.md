# Test Contracts

## TestERC20.sol

Simple ERC20 token for Web3E hardware testing on Sepolia testnet.

### Features
- **Open mint**: Anyone can call `mint(address, uint256)` - no access control
- 18 decimals (standard)
- Standard ERC20 interface (transfer, approve, transferFrom, balanceOf)
- Burn function for cleanup

### Deployment Instructions (via Remix)

1. **Open Remix IDE**: https://remix.ethereum.org/
2. **Create new file**: `TestERC20.sol`
3. **Paste contract code** from this directory
4. **Install OpenZeppelin**:
   - In Remix, the import will auto-fetch from GitHub
   - Uses `@openzeppelin/contracts@^4.9.0`
5. **Compile**:
   - Set Solidity compiler to `0.8.20` or higher
   - Enable optimization (200 runs)
6. **Deploy**:
   - Connect MetaMask to **Sepolia testnet**
   - Ensure you have Sepolia ETH for gas: https://sepoliafaucet.com/
   - Deploy > Environment: "Injected Provider - MetaMask"
   - Constructor parameters:
     - `name`: `"Web3E Test Token"`
     - `symbol`: `"W3ET"`
   - Click **Deploy** and confirm transaction
7. **Save contract address**:
   - Copy deployed contract address
   - Add to `tests/test_config.ini`:
     ```ini
     TEST_TOKEN_CONTRACT=0xYourContractAddressHere
     ```

### Required Contract ABI (for Web3E tests)

The hardware tests use these functions:

```solidity
// View functions
function name() external view returns (string memory);
function symbol() external view returns (string memory);
function decimals() external view returns (uint8);
function balanceOf(address account) external view returns (uint256);

// State-changing functions
function mint(address to, uint256 amount) external;
function transfer(address to, uint256 amount) external returns (bool);
```

### Test Usage

Tests will:
1. **Mint tokens**: Call `mint(TEST_PRIVATE_KEY_ADDRESS, 1000000000000000000)` (1 token)
2. **Check balance**: Call `balanceOf(TEST_PRIVATE_KEY_ADDRESS)`
3. **Transfer tokens**: Call `transfer(TEST_TO_ADDRESS, 100000000000000000)` (0.1 tokens)
4. **Verify receipt**: Check transaction receipt for success
5. **Verify balances**: Check both sender and recipient balances changed

### Security Note

**⚠️ WARNING**: This contract has NO ACCESS CONTROL on mint. Anyone can mint unlimited tokens.

**USE ONLY ON TESTNETS** - This is intentional for testing, but would be a critical vulnerability on mainnet.

### Gas Estimates

- Deploy: ~1,200,000 gas (~0.0024 ETH on Sepolia at 2 Gwei)
- Mint: ~50,000 gas
- Transfer: ~50,000 gas

### Example Interaction (via Remix)

After deployment, test the contract:

```javascript
// Mint 10 tokens to your address
mint("0xYourAddress", "10000000000000000000")

// Check balance (should return 10000000000000000000)
balanceOf("0xYourAddress")

// Transfer 1 token
transfer("0xRecipientAddress", "1000000000000000000")
```

### Alternative: Deploy via Hardhat/Foundry

If you prefer command-line deployment:

```bash
# Hardhat
npx hardhat run scripts/deploy-test-erc20.js --network sepolia

# Foundry
forge create TestERC20 --constructor-args "Web3E Test Token" "W3ET" \
  --rpc-url $SEPOLIA_RPC --private-key $PRIVATE_KEY
```

### Verification on Etherscan

After deployment, verify on Sepolia Etherscan:
1. Go to https://sepolia.etherscan.io/address/YOUR_CONTRACT_ADDRESS
2. Click "Contract" → "Verify and Publish"
3. Compiler version: `0.8.20`
4. Optimization: Yes (200 runs)
5. Paste flattened source (Remix can flatten for you)
6. Submit for verification

Verified contracts are easier to interact with in block explorers.
