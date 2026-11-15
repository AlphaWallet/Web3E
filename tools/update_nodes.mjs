#!/usr/bin/env node

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import tls from 'tls';
import { URL } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const FEED_URL = 'https://pub-947f58bf7fb442f7a0d0686fcf757d76.r2.dev/rpc-rankings.json';
const NODES_H_PATH = path.join(__dirname, '..', 'src', 'nodes.h');
const CERTIFICATES_H_PATH = path.join(__dirname, '..', 'src', 'Certificates.h');
const INFURA_CHAINS_PATH = path.join(__dirname, 'infura-chains.json');
const RETIRED_CHAINS_PATH = path.join(__dirname, 'retired-chains.json');
const MAX_URLS_PER_CHAIN = 3;

const isDryRun = process.argv.includes('--dry-run') || process.argv.includes('--check');
const skipCerts = process.argv.includes('--skip-certs');

// Load Infura chains config
let infuraChains = new Map();
try {
  const infuraConfig = JSON.parse(fs.readFileSync(INFURA_CHAINS_PATH, 'utf8'));
  for (const chain of infuraConfig.chains) {
    infuraChains.set(chain.chainId, chain.host);
  }
} catch (error) {
  console.warn('Warning: Could not load infura-chains.json, Infura support disabled');
}

// Load retired chains denylist
let retiredChainIds = new Set();
try {
  const retiredConfig = JSON.parse(fs.readFileSync(RETIRED_CHAINS_PATH, 'utf8'));
  for (const chain of retiredConfig.deniedChainIds) {
    retiredChainIds.add(chain.chainId);
  }
} catch (error) {
  console.warn('Warning: Could not load retired-chains.json');
}

// Chain ID to constant name mapping (from chainIds.h)
const CHAIN_ID_MAP = {
  1: { id: 'MAINNET_ID', name: 'Mainnet' },
  61: { id: 'CLASSIC_ID', name: 'Ethereum Classic' },
  99: { id: 'POA_ID', name: 'POA Network' },
  42: { id: 'KOVAN_ID', name: 'Kovan (deprecated)' },
  3: { id: 'ROPSTEN_ID', name: 'Ropsten (deprecated)' },
  77: { id: 'SOKOL_ID', name: 'Sokol' },
  4: { id: 'RINKEBY_ID', name: 'Rinkeby (deprecated)' },
  100: { id: 'XDAI_ID', name: 'Gnosis Chain' },
  5: { id: 'GOERLI_ID', name: 'Goerli' },
  246529: { id: 'ARTIS_SIGMA1_ID', name: 'ARTIS Sigma1' },
  246785: { id: 'ARTIS_TAU1_ID', name: 'ARTIS Tau1' },
  97: { id: 'BINANCE_TEST_ID', name: 'BSC Testnet' },
  56: { id: 'BINANCE_MAIN_ID', name: 'BSC' },
  128: { id: 'HECO_ID', name: 'HECO' },
  256: { id: 'HECO_TEST_ID', name: 'HECO Testnet' },
  250: { id: 'FANTOM_ID', name: 'Fantom' },
  4002: { id: 'FANTOM_TEST_ID', name: 'Fantom Testnet' },
  43114: { id: 'AVALANCHE_ID', name: 'Avalanche' },
  43113: { id: 'FUJI_TEST_ID', name: 'Avalanche Fuji' },
  137: { id: 'POLYGON_ID', name: 'Polygon' },
  80001: { id: 'MUMBAI_TEST_ID', name: 'Mumbai' },
  10: { id: 'OPTIMISTIC_MAIN_ID', name: 'Optimism' },
  69: { id: 'OPTIMISTIC_KOVAN_TEST_ID_DEPRECATED', name: 'Optimism Kovan (deprecated)' },
  338: { id: 'CRONOS_TEST_ID', name: 'Cronos Testnet' },
  42161: { id: 'ARBITRUM_MAIN_ID', name: 'Arbitrum One' },
  421611: { id: 'ARBITRUM_RINKEBY_TEST_ID_DEPRECATED', name: 'Arbitrum Rinkeby (deprecated)' },
  11297108109: { id: 'PALM_ID', name: 'Palm' },
  11297108099: { id: 'PALM_TEST_ID', name: 'Palm Testnet' },
  8217: { id: 'KLAYTN_ID', name: 'Klaytn' },
  1001: { id: 'KLAYTN_BOABAB_ID', name: 'Klaytn Baobab' },
  4689: { id: 'IOTEX_MAINNET_ID', name: 'IoTeX' },
  4690: { id: 'IOTEX_TESTNET_ID', name: 'IoTeX Testnet' },
  1313161554: { id: 'AURORA_MAINNET_ID', name: 'Aurora' },
  1313161555: { id: 'AURORA_TESTNET_ID', name: 'Aurora Testnet' },
  2001: { id: 'MILKOMEDA_C1_ID', name: 'Milkomeda C1' },
  200101: { id: 'MILKOMEDA_C1_TEST_ID', name: 'Milkomeda C1 Testnet' },
  11155111: { id: 'SEPOLIA_ID', name: 'Sepolia' },
  420: { id: 'OPTIMISM_GOERLI_ID', name: 'Optimism Goerli' },
  421613: { id: 'ARBITRUM_GOERLI_ID', name: 'Arbitrum Goerli' },
  84532: { id: 'BASE_SEPOLIA_ID', name: 'Base Sepolia' },
  421614: { id: 'ARBITRUM_SEPOLIA_ID', name: 'Arbitrum Sepolia' }
};

async function fetchFeed() {
  try {
    const response = await fetch(FEED_URL);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    return await response.json();
  } catch (error) {
    console.error('Error fetching RPC rankings feed:', error.message);
    process.exit(1);
  }
}

function isValidUrl(url) {
  // Must be https
  if (!url.startsWith('https://')) {
    return false;
  }
  
  // Skip websockets
  if (url.includes('wss://') || url.includes('ws://')) {
    return false;
  }
  
  // Skip URLs with API key placeholders
  if (url.includes('${') || url.includes('{') || url.includes('YOUR_API_KEY') || url.includes('YOUR-API-KEY')) {
    return false;
  }
  
  return true;
}

function parseExistingNodesH() {
  const content = fs.readFileSync(NODES_H_PATH, 'utf8');
  const lines = content.split('\n');
  
  const existingChains = new Map();
  
  // Find existing chain mappings in the switch statement
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i].trim();
    const caseMatch = line.match(/case\s+(\w+):/);
    
    if (caseMatch) {
      const chainConstant = caseMatch[1];
      
      // Find the chain ID for this constant
      for (const [chainId, info] of Object.entries(CHAIN_ID_MAP)) {
        if (info.id === chainConstant) {
          // Look ahead for the return statement to get current URL(s)
          let j = i + 1;
          const urls = [];
          
          while (j < lines.length && !lines[j].includes('case ') && !lines[j].includes('default:')) {
            const returnMatch = lines[j].match(/return\s+(\w+);/);
            if (returnMatch) {
              urls.push(returnMatch[1]);
            }
            j++;
            if (lines[j].includes('case ') || lines[j].includes('default:')) break;
          }
          
          existingChains.set(parseInt(chainId), {
            constant: chainConstant,
            urls: urls,
            name: info.name
          });
          break;
        }
      }
    }
  }
  
  return existingChains;
}

function selectTopRpcs(rpcs) {
  const validRpcs = rpcs.filter(rpc => {
    // Skip rate-limited if field exists
    if (rpc.rateLimited === true) {
      return false;
    }
    
    return isValidUrl(rpc.url);
  });
  
  return validRpcs.slice(0, MAX_URLS_PER_CHAIN);
}

function extractHostname(url) {
  try {
    const urlObj = new URL(url);
    return urlObj.hostname;
  } catch {
    return url;
  }
}

function generateNodesH(existingChains, chainUpdates, chainsToRemove, feedData) {
  // Build set of active chain IDs (not removed)
  const activeChainIds = new Set();
  for (const [chainId] of existingChains.entries()) {
    if (!chainsToRemove.find(r => r.chainId === chainId)) {
      activeChainIds.add(chainId);
    }
  }
  
  // Read ALL existing defines from nodes.h
  const oldNodesContent = fs.readFileSync(NODES_H_PATH, 'utf8');
  
  // Extract the generated section
  const genStart = oldNodesContent.indexOf('// =============================================================================\n// GENERATED RPC NODE DEFINITIONS');
  const genEnd = oldNodesContent.indexOf('// =============================================================================\n// END GENERATED SECTION');
  
  const existingDefines = new Map(); // defineName -> URL
  if (genStart !== -1 && genEnd !== -1) {
    const generatedSection = oldNodesContent.substring(genStart, genEnd);
    const defineRegex = /#define\s+(\w+)\s+"([^"]+)"/g;
    let match;
    while ((match = defineRegex.exec(generatedSection)) !== null) {
      existingDefines.set(match[1], match[2]);
    }
  }
  
  // Build set of define names that will be updated
  const updatedDefineNames = new Set();
  for (const [chainId, updates] of chainUpdates.entries()) {
    const existingInfo = existingChains.get(chainId);
    if (existingInfo) {
      for (let i = 0; i < MAX_URLS_PER_CHAIN; i++) {
        const defineName = `${existingInfo.constant}_URL${i > 0 ? '_' + (i + 1) : ''}`;
        updatedDefineNames.add(defineName);
      }
      
      // Also add Infura define if applicable
      if (infuraChains.has(chainId)) {
        const infuraDefineName = `${existingInfo.constant}_INFURA_URL`;
        updatedDefineNames.add(infuraDefineName);
      }
    }
  }
  
  // Generate URL defines
  let defines = [];
  const urlToDefine = new Map();
  const infuraDefines = []; // Infura-specific defines
  
  // First, add all existing defines that are NOT being updated
  for (const [defineName, url] of existingDefines.entries()) {
    if (!updatedDefineNames.has(defineName)) {
      defines.push({ name: defineName, url: url, preserve: true });
    }
  }
  
  // Then add updated URLs from feed and map preserved chains
  for (const [chainId, existingInfo] of existingChains.entries()) {
    const updates = chainUpdates.get(chainId);
    
    if (updates && updates.length > 0) {
      // Check if this chain has Infura support
      const hasInfura = infuraChains.has(chainId);
      
      if (hasInfura) {
        // Add Infura URL define
        const infuraUrl = infuraChains.get(chainId);
        const infuraDefineName = `${existingInfo.constant}_INFURA_URL`;
        infuraDefines.push({ name: infuraDefineName, url: infuraUrl, chainId: chainId, isInfura: true });
      }
      
      // Use updated public URLs from feed
      for (let i = 0; i < updates.length; i++) {
        const url = updates[i].url.replace('https://', '');
        const defineName = `${existingInfo.constant}_URL${i > 0 ? '_' + (i + 1) : ''}`;
        defines.push({ name: defineName, url: url, chainId: chainId, index: i });
        urlToDefine.set(`${chainId}_${i}`, defineName);
      }
    } else {
      // Keep existing URL definitions for this chain
      for (let i = 0; i < existingInfo.urls.length; i++) {
        const defineName = existingInfo.urls[i];
        // Map the chain to its preserved define
        urlToDefine.set(`${chainId}_${i}`, defineName);
      }
    }
  }
  
  // Sort defines: preserved ones first (in existing order), then new/updated ones by chainId
  defines.sort((a, b) => {
    if (a.preserve && !b.preserve) return -1;
    if (!a.preserve && b.preserve) return 1;
    if (a.preserve && b.preserve) return 0; // Keep existing order for preserved
    if (a.chainId !== b.chainId) return a.chainId - b.chainId;
    return a.index - b.index;
  });
  
  let header = `#include "chainIds.h"

#ifndef WEB3E_NODES_H
#define WEB3E_NODES_H

// =============================================================================
// GENERATED RPC NODE DEFINITIONS
// Generated at: ${feedData.generatedAt}
// Source: ${FEED_URL}
// 
// DO NOT MANUALLY EDIT THE GENERATED SECTION BELOW
// Run tools/update_nodes to update from latest rankings
// =============================================================================

// Public RPC node URLs (default)
`;
  
  // Add public defines
  for (const def of defines) {
    header += `#define ${def.name} "${def.url}"\n`;
  }
  
  // Add Infura-specific defines (opt-in only)
  if (infuraDefines.length > 0) {
    header += `
// Infura RPC URLs (opt-in via -DUSING_INFURA)
#ifdef USING_INFURA
`;
    for (const def of infuraDefines) {
      header += `#define ${def.name} "${def.url}"\n`;
    }
    header += `#endif // USING_INFURA
`;
  }
  
  header += `
// =============================================================================
// END GENERATED SECTION
// =============================================================================

`;
  
  // Generate getNode function
  header += `const char* getNode(long long chainId)
{
    switch (chainId)
    {
`;
  
  // Sort chains by ID for deterministic output
  const sortedChains = Array.from(existingChains.entries()).sort((a, b) => a[0] - b[0]);
  
  // Build set of removed chain IDs
  const removedChainIds = new Set(chainsToRemove.map(r => r.chainId));
  
  for (const [chainId, info] of sortedChains) {
    // Skip removed chains
    if (removedChainIds.has(chainId)) {
      continue;
    }
    
    const updates = chainUpdates.get(chainId);
    const hasInfura = infuraChains.has(chainId) && updates && updates.length > 0;
    let primaryDefine;
    
    if (updates && updates.length > 0) {
      // Chain has updates from feed - use new URL (or Infura if enabled)
      primaryDefine = urlToDefine.get(`${chainId}_0`);
    } else {
      // Chain not in feed - keep existing URL define
      // urlToDefine should have been populated for this chain too
      primaryDefine = urlToDefine.get(`${chainId}_0`);
      if (!primaryDefine) {
        // Fallback to the direct define name
        primaryDefine = info.urls[0];
      }
    }
    
    header += `    case ${info.constant}:\n`;
    
    if (hasInfura) {
      // Use Infura URL when USING_INFURA is defined, otherwise public URL
      const infuraDefineName = `${info.constant}_INFURA_URL`;
      header += `#ifdef USING_INFURA\n`;
      header += `        return ${infuraDefineName};\n`;
      header += `#else\n`;
      header += `        return ${primaryDefine};\n`;
      header += `#endif\n`;
    } else {
      header += `        return ${primaryDefine};\n`;
    }
  }
  
  header += `    default:
        return NULL;
    }
}

`;
  
  // Generate getNodeUrls function for alternate URLs
  header += `// Get all available URLs for a chain (up to 3)
// Returns primary URL at index 0
// Returns NULL when index is out of range
const char* getNodeUrls(long long chainId, int index)
{
    if (index < 0 || index >= ${MAX_URLS_PER_CHAIN}) return NULL;
    
    switch (chainId)
    {
`;
  
  for (const [chainId, info] of sortedChains) {
    const updates = chainUpdates.get(chainId);
    const urlCount = updates ? updates.length : Math.min(info.urls.length, MAX_URLS_PER_CHAIN);
    
    if (urlCount > 1) {
      header += `    case ${info.constant}:\n`;
      header += `        switch (index)\n`;
      header += `        {\n`;
      
      for (let i = 0; i < urlCount; i++) {
        const defineName = urlToDefine.get(`${chainId}_${i}`);
        header += `        case ${i}:\n`;
        header += `            return ${defineName};\n`;
      }
      
      header += `        default:\n`;
      header += `            return NULL;\n`;
      header += `        }\n`;
    } else {
      header += `    case ${info.constant}:\n`;
      header += `        return index == 0 ? getNode(chainId) : NULL;\n`;
    }
  }
  
  header += `    default:
        return NULL;
    }
}

#endif
`;
  
  return header;
}

function detectChanges(oldContent, newContent, existingChains, chainUpdates, feedData) {
  const changes = [];
  
  for (const [chainId, info] of existingChains.entries()) {
    const updates = chainUpdates.get(chainId);
    
    if (updates && updates.length > 0) {
      // Extract old primary URL
      const oldDefineMatch = oldContent.match(
        new RegExp(`#define\\s+${info.urls[0]}\\s+"([^"]+)"`)
      );
      
      const oldUrl = oldDefineMatch ? `https://${oldDefineMatch[1]}` : 'unknown';
      const newUrl = updates[0].url;
      const responseTime = updates[0].responseTimeMs;
      
      if (oldUrl !== newUrl) {
        changes.push({
          chainId: chainId,
          name: info.name,
          oldUrl: oldUrl,
          newUrl: newUrl,
          responseTime: responseTime
        });
      }
    }
  }
  
  return changes;
}

async function fetchRootCertificate(hostname) {
  return new Promise((resolve, reject) => {
    const options = {
      host: hostname,
      port: 443,
      servername: hostname, // SNI - critical for modern TLS
      rejectUnauthorized: false, // We want to fetch the cert even if it doesn't validate
      requestCert: false,
      agent: false,
      checkServerIdentity: () => undefined // Skip hostname validation
    };

    const timeout = setTimeout(() => {
      if (socket) socket.destroy();
      reject(new Error(`Timeout connecting to ${hostname}`));
    }, 15000);

    const socket = tls.connect(options, () => {
      clearTimeout(timeout);
      
      try {
        const cert = socket.getPeerCertificate(true);
        
        if (!cert || !cert.raw) {
          socket.end();
          reject(new Error(`No certificate from ${hostname}`));
          return;
        }

        // Walk the chain to find the root CA
        let current = cert;
        let rootCert = null;
        const seenFingerprints = new Set();
        
        while (current && current.raw) {
          const fingerprint = current.fingerprint;
          
          // Detect self-signed (root) certificate
          if (current.issuerCertificate) {
            if (current.issuerCertificate.fingerprint === fingerprint) {
              rootCert = current;
              break;
            }
            
            // Prevent infinite loops
            if (seenFingerprints.has(current.issuerCertificate.fingerprint)) {
              rootCert = current;
              break;
            }
            
            seenFingerprints.add(fingerprint);
            current = current.issuerCertificate;
          } else {
            rootCert = current;
            break;
          }
        }

        if (!rootCert) {
          socket.end();
          reject(new Error(`Could not find root certificate for ${hostname}`));
          return;
        }

        // Convert DER to PEM
        const derBuffer = rootCert.raw;
        const base64Cert = derBuffer.toString('base64');
        
        // Split into 64-character lines
        const lines = [];
        for (let i = 0; i < base64Cert.length; i += 64) {
          lines.push(base64Cert.substring(i, i + 64));
        }
        
        const pem = '-----BEGIN CERTIFICATE-----\n' + 
                    lines.join('\n') + '\n' +
                    '-----END CERTIFICATE-----';

        socket.end();
        resolve({
          pem: pem,
          subject: rootCert.subject,
          issuer: rootCert.issuer,
          fingerprint: rootCert.fingerprint
        });
      } catch (error) {
        socket.end();
        reject(error);
      }
    });

    socket.on('error', (error) => {
      clearTimeout(timeout);
      reject(error);
    });
  });
}

async function fetchCertificatesForChains(chainUpdates, existingChains) {
  if (skipCerts) {
    console.log('\nSkipping certificate fetch (--skip-certs)\n');
    return new Map();
  }

  console.log('\nFetching TLS certificates for updated chains...');
  console.log('(This requires network access and may take a minute)\n');
  
  const certMap = new Map(); // chainId -> cert info
  const uniqueCerts = new Map(); // fingerprint -> { pem, name, hosts }
  
  for (const [chainId, rpcs] of chainUpdates.entries()) {
    const chainInfo = existingChains.get(chainId);
    const primaryUrl = rpcs[0].url;
    const hostname = extractHostname(primaryUrl);
    
    try {
      console.log(`  Chain ${chainId} (${chainInfo.name}): Fetching cert for ${hostname}...`);
      const certInfo = await fetchRootCertificate(hostname);
      
      const fingerprint = certInfo.fingerprint;
      
      if (!uniqueCerts.has(fingerprint)) {
        // Generate a short name for this cert based on the subject
        const subjectCN = certInfo.subject.CN || certInfo.subject.O || 'Unknown';
        const certName = subjectCN
          .replace(/[^a-zA-Z0-9]/g, '_')
          .toLowerCase()
          .substring(0, 30);
        
        uniqueCerts.set(fingerprint, {
          pem: certInfo.pem,
          name: certName,
          hosts: [hostname],
          subject: certInfo.subject,
          issuer: certInfo.issuer
        });
      } else {
        const existing = uniqueCerts.get(fingerprint);
        if (!existing.hosts.includes(hostname)) {
          existing.hosts.push(hostname);
        }
      }
      
      certMap.set(chainId, fingerprint);
      
    } catch (error) {
      console.error(`    ERROR: Could not fetch certificate: ${error.message}`);
      console.error(`    Chain ${chainId} will use insecure mode (no pinned cert)`);
    }
  }
  
  console.log(`\nFetched ${uniqueCerts.size} unique root certificate(s)\n`);
  
  return { certMap, uniqueCerts };
}

function formatCertificateConstant(pem, name, index) {
  const lines = pem.split('\n').filter(line => line.length > 0);
  
  let result = `const char* ${name}_cert`;
  if (index > 0) result += `_${index}`;
  result += ' = ';
  
  for (let i = 0; i < lines.length; i++) {
    if (i === 0) {
      result += `"${lines[i]}\\n"`;
    } else {
      result += ` \\\n"${lines[i]}\\n"`;
    }
  }
  
  result += ';\n';
  return result;
}

function generateCertificatesH(existingChains, certMap, uniqueCerts, feedData, chainsToRemove) {
  let header = `#ifndef WEB3E_CERTS_H
#define WEB3E_CERTS_H

#include "chainIds.h"

// =============================================================================
// GENERATED ROOT CA CERTIFICATES
// Generated at: ${feedData.generatedAt}
// Source: ${FEED_URL}
// 
// Root CAs extracted via TLS connection to each RPC endpoint's hostname.
// DO NOT MANUALLY EDIT THE GENERATED SECTION BELOW
// Run tools/update_nodes to update from latest rankings
// =============================================================================

`;

  // Generate certificate constants
  const certsByName = new Map();
  let certIndex = 0;
  
  for (const [fingerprint, certInfo] of uniqueCerts.entries()) {
    const baseName = certInfo.name;
    let finalName = baseName;
    let counter = 0;
    
    // Ensure unique names
    while (certsByName.has(finalName)) {
      counter++;
      finalName = `${baseName}_${counter}`;
    }
    
    certsByName.set(finalName, fingerprint);
    certInfo.finalName = finalName;
    
    header += `// Root CA for: ${certInfo.hosts.join(', ')}\n`;
    header += `// Subject: CN=${certInfo.subject.CN || certInfo.subject.O || 'N/A'}\n`;
    header += formatCertificateConstant(certInfo.pem, finalName, 0);
    header += '\n';
  }

  // Add Infura root CA
  const infuraRootCA = `-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----`;

  header += `
// Infura root certificate (opt-in via -DUSING_INFURA)
#ifdef USING_INFURA
// Root CA for Infura (Amazon Root CA 1)
`;
  header += formatCertificateConstant(infuraRootCA, 'infura_root_ca', 0);
  header += `#endif // USING_INFURA

// =============================================================================
// END GENERATED SECTION
// =============================================================================

/**
 * @brief Get the root CA certificate for a chain ID
 * 
 * Returns the pinned root CA certificate for the primary RPC endpoint of the
 * given chain. If no certificate is available, returns NULL and Web3::setupCert
 * will use insecure mode (not recommended for mainnet).
 * 
 * @param chainId Chain ID (e.g., MAINNET_ID, SEPOLIA_ID)
 * @return const char* PEM-encoded root CA certificate, or NULL if unavailable
 */
const char* getCertificate(long long chainId)
{
    switch (chainId)
    {
`;

  // Sort chains by ID for deterministic output
  const sortedChains = Array.from(existingChains.entries()).sort((a, b) => a[0] - b[0]);
  
  // Build set of removed chain IDs
  const removedChainIds = new Set(chainsToRemove.map(r => r.chainId));
  
  for (const [chainId, info] of sortedChains) {
    // Skip removed chains
    if (removedChainIds.has(chainId)) {
      continue;
    }
    
    const fingerprint = certMap.get(chainId);
    const hasInfura = infuraChains.has(chainId);
    
    if (fingerprint && uniqueCerts.has(fingerprint)) {
      const certInfo = uniqueCerts.get(fingerprint);
      header += `    case ${info.constant}:\n`;
      
      if (hasInfura) {
        header += `#ifdef USING_INFURA\n`;
        header += `        return infura_root_ca_cert;\n`;
        header += `#else\n`;
        header += `        return ${certInfo.finalName}_cert;\n`;
        header += `#endif\n`;
      } else {
        header += `        return ${certInfo.finalName}_cert;\n`;
      }
    } else if (hasInfura) {
      // Chain has no public cert yet but has Infura
      header += `    case ${info.constant}:\n`;
      header += `#ifdef USING_INFURA\n`;
      header += `        return infura_root_ca_cert;\n`;
      header += `#else\n`;
      header += `        return NULL; // No public certificate available\n`;
      header += `#endif\n`;
    }
  }

  header += `    default:
        return NULL;
    }
}

#endif
`;

  return header;
}

async function main() {
  console.log('Web3E RPC Node Updater');
  console.log('======================\n');
  
  if (isDryRun) {
    console.log('Running in DRY RUN mode - no files will be modified\n');
  }
  
  if (skipCerts) {
    console.log('Note: Certificate fetching will be skipped (--skip-certs)\n');
  }
  
  console.log('Fetching RPC rankings feed...');
  const feedData = await fetchFeed();
  console.log(`Feed generated at: ${feedData.generatedAt}`);
  console.log(`Feed contains ${feedData.chains.length} chains\n`);
  
  console.log('Parsing existing nodes.h...');
  const existingChains = parseExistingNodesH();
  console.log(`Found ${existingChains.size} chains in nodes.h`);
  
  // Debug: Check SOKOL
  const sokol = existingChains.get(77);
  if (sokol) {
    console.log(`  Debug SOKOL (77): constant=${sokol.constant}, urls=[${sokol.urls.join(', ')}]`);
  }
  console.log('');
  
  console.log('Processing updates...');
  const chainUpdates = new Map();
  const chainsToRemove = [];
  let totalUpdates = 0;
  
  // Build feed chain set
  const feedChainIds = new Set();
  for (const chain of feedData.chains) {
    const topRpcs = selectTopRpcs(chain.rpcs);
    if (topRpcs.length > 0) {
      feedChainIds.add(chain.chainId);
    }
  }
  
  // Process each existing chain
  for (const [chainId, chainInfo] of existingChains.entries()) {
    // Check retired/denied chains first
    if (retiredChainIds.has(chainId)) {
      chainsToRemove.push({
        chainId: chainId,
        name: chainInfo.name,
        reason: 'Retired/reassigned chain ID'
      });
      continue;
    }
    
    // Check if chain has usable RPCs in feed
    if (!feedChainIds.has(chainId)) {
      chainsToRemove.push({
        chainId: chainId,
        name: chainInfo.name,
        reason: 'No usable RPCs in feed'
      });
      continue;
    }
    
    // Chain is valid, process updates
    for (const chain of feedData.chains) {
      if (chain.chainId === chainId) {
        const topRpcs = selectTopRpcs(chain.rpcs);
        if (topRpcs.length > 0) {
          chainUpdates.set(chainId, topRpcs);
          totalUpdates++;
          console.log(`  Chain ${chainId} (${chainInfo.name}): ${topRpcs.length} URL(s)`);
          topRpcs.forEach((rpc, i) => {
            console.log(`    ${i + 1}. ${rpc.url} (${rpc.responseTimeMs.toFixed(2)}ms)`);
          });
        }
        break;
      }
    }
  }
  
  console.log(`\nProcessed ${totalUpdates} chain(s) with available RPCs`);
  
  if (chainsToRemove.length > 0) {
    console.log(`\nChains to remove (${chainsToRemove.length}):`);
    for (const removed of chainsToRemove) {
      console.log(`  - Chain ${removed.chainId} (${removed.name}): ${removed.reason}`);
    }
  }
  console.log('');
  
  // Read existing content
  const oldContent = fs.readFileSync(NODES_H_PATH, 'utf8');
  
  // Generate new content
  const newContent = generateNodesH(existingChains, chainUpdates, chainsToRemove, feedData);
  
  // Detect changes
  const changes = detectChanges(oldContent, newContent, existingChains, chainUpdates, feedData);
  
  console.log('Changes detected:');
  if (changes.length === 0) {
    console.log('  No changes to primary URLs\n');
  } else {
    console.log(`  ${changes.length} chain(s) with primary URL changes:\n`);
    for (const change of changes) {
      console.log(`  Chain ${change.chainId} (${change.name}):`);
      console.log(`    Old: ${change.oldUrl}`);
      console.log(`    New: ${change.newUrl} (${change.responseTime.toFixed(2)}ms)\n`);
    }
  }
  
  // Fetch certificates for chains with updates
  let certMap = new Map();
  let uniqueCerts = new Map();
  
  if (!skipCerts) {
    const certResult = await fetchCertificatesForChains(chainUpdates, existingChains);
    certMap = certResult.certMap;
    uniqueCerts = certResult.uniqueCerts;
  }
  
  // Generate new Certificates.h
  const newCertificatesContent = generateCertificatesH(existingChains, certMap, uniqueCerts, feedData, chainsToRemove);
  
  // Read old Certificates.h to detect changes
  const oldCertificatesContent = fs.existsSync(CERTIFICATES_H_PATH) 
    ? fs.readFileSync(CERTIFICATES_H_PATH, 'utf8')
    : '';
  
  const certsChanged = oldCertificatesContent !== newCertificatesContent;
  
  if (isDryRun) {
    console.log('Dry run complete. Use without --check/--dry-run to apply changes.');
    const anyChanges = changes.length > 0 || certsChanged;
    if (certsChanged) {
      console.log(`\nCertificates.h would be updated with ${uniqueCerts.size} root CA(s)`);
    }
    process.exit(anyChanges ? 1 : 0);
  } else {
    console.log('Writing updated nodes.h...');
    fs.writeFileSync(NODES_H_PATH, newContent, 'utf8');
    console.log('Done!');
    
    if (!skipCerts) {
      console.log('\nWriting updated Certificates.h...');
      fs.writeFileSync(CERTIFICATES_H_PATH, newCertificatesContent, 'utf8');
      console.log('Done!');
      console.log(`\nGenerated ${uniqueCerts.size} unique root certificate(s)`);
    }
    
    console.log('');
    process.exit(0);
  }
}

main().catch(error => {
  console.error('Fatal error:', error);
  process.exit(1);
});
