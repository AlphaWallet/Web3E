// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";

/**
 * @title TestERC20
 * @dev Simple ERC20 token for Web3E testing on Sepolia testnet
 * 
 * Features:
 * - Open mint: Anyone can mint tokens for testing (NO ACCESS CONTROL)
 * - 18 decimals (standard)
 * - Standard ERC20 transfer/approve/transferFrom
 * 
 * Deploy via Remix:
 * 1. Go to https://remix.ethereum.org/
 * 2. Create new file, paste this contract
 * 3. Set compiler to 0.8.20+
 * 4. Connect MetaMask to Sepolia testnet
 * 5. Deploy with constructor args: "Web3E Test Token", "W3ET"
 * 6. Copy deployed contract address to tests/test_config.ini as TEST_TOKEN_CONTRACT
 * 
 * Required ABI for tests (minimal):
 * - function mint(address to, uint256 amount) external
 * - function transfer(address to, uint256 amount) external returns (bool)
 * - function balanceOf(address account) external view returns (uint256)
 * - function decimals() external view returns (uint8)
 * - function name() external view returns (string)
 * - function symbol() external view returns (string)
 */
contract TestERC20 is ERC20 {
    constructor(string memory name, string memory symbol) ERC20(name, symbol) {}
    
    /**
     * @dev Open mint function - anyone can mint for testing
     * @param to Address to mint tokens to
     * @param amount Amount of tokens to mint (in base units, not considering decimals)
     * 
     * Example: mint(0x123..., 1000000000000000000) mints 1 token (18 decimals)
     */
    function mint(address to, uint256 amount) external {
        _mint(to, amount);
    }
    
    /**
     * @dev Burn function for cleanup
     * @param amount Amount to burn from caller's balance
     */
    function burn(uint256 amount) external {
        _burn(msg.sender, amount);
    }
}
