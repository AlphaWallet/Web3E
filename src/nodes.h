#ifndef APLOEMBED_NODES_H
#define APLOEMBED_NODES_H

// AploCoin RPC endpoints
// NOTE: pub1 and pub2 are planned hostnames - do NOT use for live requests yet
#define APLO_PRIMARY_RPC_URL "pub1.aplocoin.com"
#define APLO_FALLBACK_RPC_URL "pub2.aplocoin.com"

// Get primary Aplo RPC node
const char* getAploPrimaryNode()
{
    return APLO_PRIMARY_RPC_URL;
}

// Get fallback Aplo RPC node
const char* getAploFallbackNode()
{
    return APLO_FALLBACK_RPC_URL;
}

#endif
