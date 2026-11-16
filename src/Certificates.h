#ifndef WEB3E_CERTS_H
#define WEB3E_CERTS_H

#include "chainIds.h"
#include "nodes.h"

// =============================================================================
// TLS CERTIFICATE CONFIGURATION
// 
// Public RPCs: Use unverified TLS (HTTPS without server verification)
// Infura (opt-in): Use pinned Amazon Root CA 1 for verified TLS
// 
// To enable Infura with verified TLS, compile with:
//   -DUSING_INFURA -DINFURA_API_KEY="your-key"
// =============================================================================

#ifdef USING_INFURA
// Root CA for Infura (Amazon Root CA 1)
// Used for all Infura endpoints: mainnet.infura.io, polygon-mainnet.infura.io, etc.
static const char* const infura_root_ca_cert = "-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";
#endif // USING_INFURA

/**
 * @brief Get the root CA certificate for TLS verification
 * 
 * Returns Amazon Root CA 1 when USING_INFURA is defined and the selected
 * node is an infura.io host. Otherwise returns NULL, causing Web3::setupCert()
 * to use unverified TLS (client->setInsecure()).
 * 
 * Public RPCs use unverified TLS for simplicity and compatibility.
 * Infura uses pinned certificate verification for enhanced security.
 * 
 * @param chainId Chain ID (used to determine selected host via getNode())
 * @return const char* Amazon Root CA 1 for Infura, NULL for public RPCs
 */
inline const char* getCertificate(long long chainId)
{
#ifdef USING_INFURA
    // Check if the selected host is an Infura endpoint
    const char* node = getNode(chainId);
    if (node != NULL && strlen(node) > 0 && strstr(node, "infura.io") != NULL) {
        return infura_root_ca_cert;
    }
#endif
    
    // Public RPCs use unverified TLS (NULL triggers client->setInsecure())
    return NULL;
}

#endif
