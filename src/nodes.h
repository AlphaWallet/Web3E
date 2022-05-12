#include "chainIds.h"

#ifndef WEB3E_NODES_H
#define WEB3E_NODES_H

#ifdef USING_INFURA
#define MAINNET_RPC_URL "mainnet.infura.io/v3/"
#define RINKEBY_RPC_URL "rinkeby.infura.io/v3/"
#define KOVAN_RPC_URL "kovan.infura.io/v3/"
#define GOERLI_RPC_URL "goerli.infura.io/v3/"
#define POLYGON_RPC_URL "polygon-mainnet.infura.io/v3/"
#define ARBITRUM_RPC_URL "arbitrum-mainnet.infura.io/v3/"
#define MUMBAI_RPC_URL "polygon-mumbai.infura.io/v3/"
#define OPTIMISM_RPC_URL "optimism-mainnet.infura.io/v3/"
#define OPTIMISM_TESTRPC_URL "optimism-kovan.infura.io/v3/"
#define ARBITRUM_TEST_RPC_URL "arbitrum-rinkeby.infura.io/v3/"
#define PALM_RPC_URL "palm-mainnet.infura.io/v3/"
#define PALM_TEST_RPC_URL "palm-testnet.infura.io/v3/"
#else
#define MAINNET_RPC_URL "main-rpc.linkpool.io"
#define POLYGON_RPC_URL "polygon-rpc.com"
#define ARBITRUM_RPC_URL "arbitrum.public-rpc.com"
#define RINKEBY_RPC_URL "rinkeby-light.eth.linkpool.io"
#define GOERLI_RPC_URL "goerli-light.eth.linkpool.io"
#define MUMBAI_RPC_URL "rpc-mumbai.maticvigil.com"
#define OPTIMISM_RPC_URL "mainnet.optimism.io"
#define ARBITRUM_TEST_RPC_URL "rinkeby.arbitrum.io/rpc"
#define KOVAN_RPC_URL "kovan.poa.network"
#define OPTIMISM_TESTRPC_URL "kovan.optimism.io"
#define PALM_RPC_URL "palm-mainnet.infura.io/v3/3a961d6501e54add9a41aa53f15de99b"
#define PALM_TEST_RPC_URL "palm-testnet.infura.io/v3/3a961d6501e54add9a41aa53f15de99b"
#endif

#define CLASSIC_RPC_URL "www.ethercluster.com/etc"
#define XDAI_RPC_URL "rpc.xdaichain.com"
#define POA_RPC_URL "core.poa.network"
#define SOKOL_RPC_URL "sokol.poa.network"
#define ARTIS_SIGMA1_RPC_URL "rpc.sigma1.artis.network"
#define ARTIS_TAU1_RPC_URL "rpc.tau1.artis.network"
#define BINANCE_TEST_RPC_URL "data-seed-prebsc-1-s3.binance.org:8545"
#define BINANCE_MAIN_RPC_URL "bsc-dataseed.binance.org"
#define HECO_RPC_URL "http-mainnet.hecochain.com"
#define HECO_TEST_RPC_URL "http-testnet.hecochain.com"
#define CRONOS_TEST_URL "cronos-testnet.crypto.org:8545"
#define IOTEX_MAINNET_RPC_URL "babel-api.mainnet.iotex.io"
#define IOTEX_TESTNET_RPC_URL "babel-api.testnet.iotex.io"
#define AURORA_MAINNET_RPC_URL "mainnet.aurora.dev"
#define AURORA_TESTNET_RPC_URL "testnet.aurora.dev"
#define MILKOMEDA_C1_RPC "rpc-mainnet-cardano-evm.c1.milkomeda.com"
#define MILKOMEDA_C1_TEST_RPC "rpc-devnet-cardano-evm.c1.milkomeda.com"
#define KLAYTN_RPC "public-node-api.klaytnapi.com/v1/cypress"
#define KLAYTN_BAOBAB_RPC "api.baobab.klaytn.net:8651"

const char* getNode(long long chainId)
{
    switch (chainId)
    {
    case MAINNET_ID:
        return MAINNET_RPC_URL;
    case CLASSIC_ID:
        return CLASSIC_RPC_URL;
    case POA_ID:
        return POA_RPC_URL;
    case KOVAN_ID:
        return KOVAN_RPC_URL;
    case SOKOL_ID:
        return SOKOL_RPC_URL;
    case RINKEBY_ID:
        return RINKEBY_RPC_URL;
    case XDAI_ID:
        return XDAI_RPC_URL;
    case GOERLI_ID:
        return GOERLI_RPC_URL;
    case KLAYTN_ID:
        return KLAYTN_RPC;
    case KLAYTN_BOABAB_ID:
        return KLAYTN_BAOBAB_RPC;
    case IOTEX_MAINNET_ID:
        return IOTEX_MAINNET_RPC_URL;
    case IOTEX_TESTNET_ID:
        return IOTEX_TESTNET_RPC_URL;
    case POLYGON_ID:
        return POLYGON_RPC_URL;
    case MUMBAI_TEST_ID:
        return MUMBAI_RPC_URL;
    case MILKOMEDA_C1_ID:
        return MILKOMEDA_C1_RPC;
    case MILKOMEDA_C1_TEST_ID:
        return MILKOMEDA_C1_TEST_RPC;

    default:
        return "";
    }
}

#endif