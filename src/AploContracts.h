#ifndef APLOEMBED_CONTRACTS_H
#define APLOEMBED_CONTRACTS_H

// AploCoin Chain ID (from aplonode/params/config.go)
#define APLO_ID 28282

// AploCoin Smart Contract Addresses
// These are example addresses from WebMiner reference implementation
// Update these with actual deployed contract addresses for your network

// Mining contract address (handles mine() function and miner_params)
#define APLO_MINING_CONTRACT "0x0000000000000000000000000000000000001234"

// Staking contract address (handles stake/unstake/getStake/getMultiplier)
#define APLO_STAKING_CONTRACT "0x0000000000000000000000000000000000001235"

// Minimum stake amount in APLO (converted to Gaplo/wei at runtime)
#define APLO_MIN_STAKE_AMOUNT 1000

// Helper function to get mining contract address
inline const char* getAploMiningContract() {
    return APLO_MINING_CONTRACT;
}

// Helper function to get staking contract address
inline const char* getAploStakingContract() {
    return APLO_STAKING_CONTRACT;
}

#endif // APLOEMBED_CONTRACTS_H
