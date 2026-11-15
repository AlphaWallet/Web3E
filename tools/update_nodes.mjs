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
const MAX_URLS_PER_CHAIN = 3;

const isDryRun = process.argv.includes('--dry-run') || process.argv.includes('--check');
const skipCerts = process.argv.includes('--skip-certs');

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

function generateNodesH(existingChains, feedData) {
  const chainUpdates = new Map();
  
  // Process feed data
  for (const chain of feedData.chains) {
    if (existingChains.has(chain.chainId)) {
      const topRpcs = selectTopRpcs(chain.rpcs);
      if (topRpcs.length > 0) {
        chainUpdates.set(chain.chainId, topRpcs);
      }
    }
  }
  
  // Generate URL defines
  let defines = [];
  const urlToDefine = new Map();
  let defineCounter = 0;
  
  for (const [chainId, existingInfo] of existingChains.entries()) {
    const updates = chainUpdates.get(chainId);
    
    if (updates && updates.length > 0) {
      // Use updated URLs from feed
      for (let i = 0; i < updates.length; i++) {
        const url = updates[i].url.replace('https://', '');
        const defineName = `${existingInfo.constant}_URL${i > 0 ? '_' + (i + 1) : ''}`;
        defines.push({ name: defineName, url: url, chainId: chainId, index: i });
        urlToDefine.set(`${chainId}_${i}`, defineName);
      }
    } else {
      // Keep existing (placeholder or Infura URLs that need API keys)
      for (let i = 0; i < existingInfo.urls.length; i++) {
        const defineName = existingInfo.urls[i];
        defines.push({ name: defineName, url: null, chainId: chainId, index: i, keepExisting: true });
        urlToDefine.set(`${chainId}_${i}`, defineName);
      }
    }
  }
  
  // Sort defines by chain ID for deterministic output
  defines.sort((a, b) => {
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

`;
  
  // Add defines
  for (const def of defines) {
    if (def.keepExisting) {
      // Read the existing define from the file
      const content = fs.readFileSync(NODES_H_PATH, 'utf8');
      const defineMatch = content.match(new RegExp(`#define\\s+${def.name}\\s+"([^"]+)"`));
      if (defineMatch) {
        header += `#define ${def.name} "${defineMatch[1]}"\n`;
      }
    } else {
      header += `#define ${def.name} "${def.url}"\n`;
    }
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
  
  for (const [chainId, info] of sortedChains) {
    const updates = chainUpdates.get(chainId);
    const primaryDefine = updates && updates.length > 0 
      ? urlToDefine.get(`${chainId}_0`)
      : info.urls[0];
    
    header += `    case ${info.constant}:\n`;
    header += `        return ${primaryDefine};\n`;
  }
  
  header += `    default:
        return "";
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

function generateCertificatesH(existingChains, certMap, uniqueCerts, feedData) {
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

  header += `// =============================================================================
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
  
  for (const [chainId, info] of sortedChains) {
    const fingerprint = certMap.get(chainId);
    
    if (fingerprint && uniqueCerts.has(fingerprint)) {
      const certInfo = uniqueCerts.get(fingerprint);
      header += `    case ${info.constant}:\n`;
      header += `        return ${certInfo.finalName}_cert;\n`;
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
  console.log(`Found ${existingChains.size} chains in nodes.h\n`);
  
  console.log('Processing updates...');
  const chainUpdates = new Map();
  let totalUpdates = 0;
  
  for (const chain of feedData.chains) {
    if (existingChains.has(chain.chainId)) {
      const topRpcs = selectTopRpcs(chain.rpcs);
      if (topRpcs.length > 0) {
        chainUpdates.set(chain.chainId, topRpcs);
        totalUpdates++;
        const chainInfo = existingChains.get(chain.chainId);
        console.log(`  Chain ${chain.chainId} (${chainInfo.name}): ${topRpcs.length} URL(s)`);
        topRpcs.forEach((rpc, i) => {
          console.log(`    ${i + 1}. ${rpc.url} (${rpc.responseTimeMs.toFixed(2)}ms)`);
        });
      }
    }
  }
  
  console.log(`\nProcessed ${totalUpdates} chain(s) with available RPCs\n`);
  
  // Read existing content
  const oldContent = fs.readFileSync(NODES_H_PATH, 'utf8');
  
  // Generate new content
  const newContent = generateNodesH(existingChains, feedData);
  
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
  const { certMap, uniqueCerts } = await fetchCertificatesForChains(chainUpdates, existingChains);
  
  // Generate new Certificates.h
  const newCertificatesContent = generateCertificatesH(existingChains, certMap, uniqueCerts, feedData);
  
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
