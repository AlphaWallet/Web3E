#include "chainIds.h"

#ifndef WEB3E_NODES_H
#define WEB3E_NODES_H

// =============================================================================
// GENERATED RPC NODE DEFINITIONS
// Generated at: 2026-11-15T17:26:31.924Z
// Source: https://pub-947f58bf7fb442f7a0d0686fcf757d76.r2.dev/rpc-rankings.json
// 
// DO NOT MANUALLY EDIT THE GENERATED SECTION BELOW
// Run tools/update_nodes to update from latest rankings
// =============================================================================

#define MAINNET_ID_URL "ethereum-rpc.publicnode.com"
#define MAINNET_ID_URL_2 "eth.drpc.org"
#define MAINNET_ID_URL_3 "rpc.flashbots.net/fast"
#define OPTIMISTIC_MAIN_ID_URL "optimism.drpc.org"
#define OPTIMISTIC_MAIN_ID_URL_2 "optimism-rpc.publicnode.com"
#define OPTIMISTIC_MAIN_ID_URL_3 "gateway.tenderly.co/public/optimism"
#define KOVAN_ID_URL "public-lukso.nownodes.io"
#define KOVAN_ID_URL_2 "rpc.mainnet.lukso.network"
#define CLASSIC_ID_URL "ethereum-classic-mainnet.gateway.tatum.io"
#define CLASSIC_ID_URL_2 "rpc.swiftnodes.io/rpc/etc"
#define CLASSIC_ID_URL_3 "0xrpc.io/etc"
#define XDAI_ID_URL "gnosis-rpc.publicnode.com"
#define XDAI_ID_URL_2 "gno-mainnet.gateway.tatum.io"
#define XDAI_ID_URL_3 "gnosis-rpc.blockreq.com/v1/rpc/public"
#define POLYGON_ID_URL "polygon-bor-rpc.publicnode.com"
#define POLYGON_ID_URL_2 "polygon.drpc.org"
#define POLYGON_ID_URL_3 "polygon.gateway.tenderly.co"
#define KLAYTN_BOABAB_ID_URL "public-en-kairos.node.kaia.io"
#define KLAYTN_BOABAB_ID_URL_2 "lb.routeme.sh/rpc/evm/1001"
#define KLAYTN_BOABAB_ID_URL_3 "klaytn-baobab.drpc.org"
#define IOTEX_MAINNET_ID_URL "babel-api.mainnet.iotex.one"
#define IOTEX_MAINNET_ID_URL_2 "babel-api.fastblocks.io"
#define IOTEX_MAINNET_ID_URL_3 "babel-api.mainnet.iotex.io"
#define IOTEX_TESTNET_ID_URL "babel-api.testnet.iotex.io"
#define KLAYTN_ID_URL "public.1rpc.io/klay"
#define KLAYTN_ID_URL_2 "public-en.node.kaia.io"
#define KLAYTN_ID_URL_3 "kaia-mainnet.gateway.tatum.io"
#define ARBITRUM_MAIN_ID_URL "arbitrum-one-rpc.publicnode.com"
#define ARBITRUM_MAIN_ID_URL_2 "arb1.arbitrum.io/rpc"
#define ARBITRUM_MAIN_ID_URL_3 "arbitrum-one.public.blastapi.io"
#define FUJI_TEST_ID_URL "api.avax-test.network/ext/bc/C/rpc"
#define FUJI_TEST_ID_URL_2 "avalanche-fuji-c-chain-rpc.publicnode.com"
#define FUJI_TEST_ID_URL_3 "avalanche-fuji.drpc.org"
#define BASE_SEPOLIA_ID_URL "base-sepolia-rpc.publicnode.com"
#define BASE_SEPOLIA_ID_URL_2 "base-sepolia.rpc.sentio.xyz"
#define BASE_SEPOLIA_ID_URL_3 "sepolia.base.org"
#define ARBITRUM_SEPOLIA_ID_URL "arbitrum-sepolia.gateway.tenderly.co"
#define ARBITRUM_SEPOLIA_ID_URL_2 "arbitrum-sepolia-rpc.publicnode.com"
#define ARBITRUM_SEPOLIA_ID_URL_3 "sepolia-rollup.arbitrum.io/rpc"
#define SEPOLIA_ID_URL "ethereum-sepolia-rpc.publicnode.com"
#define SEPOLIA_ID_URL_2 "sepolia.gateway.tenderly.co"
#define SEPOLIA_ID_URL_3 "public.1rpc.io/sepolia"

// =============================================================================
// END GENERATED SECTION
// =============================================================================

const char* getNode(long long chainId)
{
    switch (chainId)
    {
    case MAINNET_ID:
        return MAINNET_ID_URL;
    case RINKEBY_ID:
        return undefined;
    case GOERLI_ID:
        return undefined;
    case OPTIMISTIC_MAIN_ID:
        return OPTIMISTIC_MAIN_ID_URL;
    case KOVAN_ID:
        return KOVAN_ID_URL;
    case CLASSIC_ID:
        return CLASSIC_ID_URL;
    case OPTIMISTIC_KOVAN_TEST_ID_DEPRECATED:
        return undefined;
    case SOKOL_ID:
        return undefined;
    case POA_ID:
        return undefined;
    case XDAI_ID:
        return XDAI_ID_URL;
    case POLYGON_ID:
        return POLYGON_ID_URL;
    case OPTIMISM_GOERLI_ID:
        return undefined;
    case KLAYTN_BOABAB_ID:
        return KLAYTN_BOABAB_ID_URL;
    case MILKOMEDA_C1_ID:
        return undefined;
    case IOTEX_MAINNET_ID:
        return IOTEX_MAINNET_ID_URL;
    case IOTEX_TESTNET_ID:
        return IOTEX_TESTNET_ID_URL;
    case KLAYTN_ID:
        return KLAYTN_ID_URL;
    case ARBITRUM_MAIN_ID:
        return ARBITRUM_MAIN_ID_URL;
    case FUJI_TEST_ID:
        return FUJI_TEST_ID_URL;
    case MUMBAI_TEST_ID:
        return undefined;
    case BASE_SEPOLIA_ID:
        return BASE_SEPOLIA_ID_URL;
    case MILKOMEDA_C1_TEST_ID:
        return undefined;
    case ARBITRUM_RINKEBY_TEST_ID_DEPRECATED:
        return undefined;
    case ARBITRUM_GOERLI_ID:
        return undefined;
    case ARBITRUM_SEPOLIA_ID:
        return ARBITRUM_SEPOLIA_ID_URL;
    case SEPOLIA_ID:
        return SEPOLIA_ID_URL;
    default:
        return "";
    }
}

// Get all available URLs for a chain (up to 3)
// Returns primary URL at index 0
// Returns NULL when index is out of range
const char* getNodeUrls(long long chainId, int index)
{
    if (index < 0 || index >= 3) return NULL;
    
    switch (chainId)
    {
    case MAINNET_ID:
        switch (index)
        {
        case 0:
            return MAINNET_ID_URL;
        case 1:
            return MAINNET_ID_URL_2;
        case 2:
            return MAINNET_ID_URL_3;
        default:
            return NULL;
        }
    case RINKEBY_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case GOERLI_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case OPTIMISTIC_MAIN_ID:
        switch (index)
        {
        case 0:
            return OPTIMISTIC_MAIN_ID_URL;
        case 1:
            return OPTIMISTIC_MAIN_ID_URL_2;
        case 2:
            return OPTIMISTIC_MAIN_ID_URL_3;
        default:
            return NULL;
        }
    case KOVAN_ID:
        switch (index)
        {
        case 0:
            return KOVAN_ID_URL;
        case 1:
            return KOVAN_ID_URL_2;
        default:
            return NULL;
        }
    case CLASSIC_ID:
        switch (index)
        {
        case 0:
            return CLASSIC_ID_URL;
        case 1:
            return CLASSIC_ID_URL_2;
        case 2:
            return CLASSIC_ID_URL_3;
        default:
            return NULL;
        }
    case OPTIMISTIC_KOVAN_TEST_ID_DEPRECATED:
        return index == 0 ? getNode(chainId) : NULL;
    case SOKOL_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case POA_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case XDAI_ID:
        switch (index)
        {
        case 0:
            return XDAI_ID_URL;
        case 1:
            return XDAI_ID_URL_2;
        case 2:
            return XDAI_ID_URL_3;
        default:
            return NULL;
        }
    case POLYGON_ID:
        switch (index)
        {
        case 0:
            return POLYGON_ID_URL;
        case 1:
            return POLYGON_ID_URL_2;
        case 2:
            return POLYGON_ID_URL_3;
        default:
            return NULL;
        }
    case OPTIMISM_GOERLI_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case KLAYTN_BOABAB_ID:
        switch (index)
        {
        case 0:
            return KLAYTN_BOABAB_ID_URL;
        case 1:
            return KLAYTN_BOABAB_ID_URL_2;
        case 2:
            return KLAYTN_BOABAB_ID_URL_3;
        default:
            return NULL;
        }
    case MILKOMEDA_C1_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case IOTEX_MAINNET_ID:
        switch (index)
        {
        case 0:
            return IOTEX_MAINNET_ID_URL;
        case 1:
            return IOTEX_MAINNET_ID_URL_2;
        case 2:
            return IOTEX_MAINNET_ID_URL_3;
        default:
            return NULL;
        }
    case IOTEX_TESTNET_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case KLAYTN_ID:
        switch (index)
        {
        case 0:
            return KLAYTN_ID_URL;
        case 1:
            return KLAYTN_ID_URL_2;
        case 2:
            return KLAYTN_ID_URL_3;
        default:
            return NULL;
        }
    case ARBITRUM_MAIN_ID:
        switch (index)
        {
        case 0:
            return ARBITRUM_MAIN_ID_URL;
        case 1:
            return ARBITRUM_MAIN_ID_URL_2;
        case 2:
            return ARBITRUM_MAIN_ID_URL_3;
        default:
            return NULL;
        }
    case FUJI_TEST_ID:
        switch (index)
        {
        case 0:
            return FUJI_TEST_ID_URL;
        case 1:
            return FUJI_TEST_ID_URL_2;
        case 2:
            return FUJI_TEST_ID_URL_3;
        default:
            return NULL;
        }
    case MUMBAI_TEST_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case BASE_SEPOLIA_ID:
        switch (index)
        {
        case 0:
            return BASE_SEPOLIA_ID_URL;
        case 1:
            return BASE_SEPOLIA_ID_URL_2;
        case 2:
            return BASE_SEPOLIA_ID_URL_3;
        default:
            return NULL;
        }
    case MILKOMEDA_C1_TEST_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case ARBITRUM_RINKEBY_TEST_ID_DEPRECATED:
        return index == 0 ? getNode(chainId) : NULL;
    case ARBITRUM_GOERLI_ID:
        return index == 0 ? getNode(chainId) : NULL;
    case ARBITRUM_SEPOLIA_ID:
        switch (index)
        {
        case 0:
            return ARBITRUM_SEPOLIA_ID_URL;
        case 1:
            return ARBITRUM_SEPOLIA_ID_URL_2;
        case 2:
            return ARBITRUM_SEPOLIA_ID_URL_3;
        default:
            return NULL;
        }
    case SEPOLIA_ID:
        switch (index)
        {
        case 0:
            return SEPOLIA_ID_URL;
        case 1:
            return SEPOLIA_ID_URL_2;
        case 2:
            return SEPOLIA_ID_URL_3;
        default:
            return NULL;
        }
    default:
        return NULL;
    }
}

#endif
