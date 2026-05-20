#ifndef APLOEMBED_CERTS_H
#define APLOEMBED_CERTS_H

#include "chainIds.h"

// TODO: Add AploCoin-specific certificate when pub1/pub2.aplocoin.com are deployed
// For now, we'll use setInsecure() in Web3::setupCert()

const char* getCertificate(long long chainId)
{
    // Currently no certificate configured for AploCoin
    // Will return nullptr to trigger setInsecure() mode
    if (chainId == APLO_ID)
    {
        return nullptr;
    }
    
    return nullptr;
}

#endif
