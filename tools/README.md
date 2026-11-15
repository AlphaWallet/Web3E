# Web3E Tools

This directory contains build-time tools for maintaining the Web3E library.

## RPC Node Updater

The RPC node updater automatically fetches the latest RPC endpoint rankings and updates `src/nodes.h` with the fastest available public nodes.

### Files

- **`update_nodes.mjs`** - Node.js script that fetches rankings and updates nodes.h
- **`auto-update.ps1`** - PowerShell wrapper for automated updates and PR creation
- **`package.json`** - Node.js dependencies

### Requirements

- **Node.js** (v18 or later recommended)
- **Git**
- **GitHub CLI** (`gh`) - Only needed for `auto-update.ps1`

### Manual Usage

#### Check for Updates (Dry Run)

```bash
cd tools
npm install
node update_nodes.mjs --check
```

This will check for available updates without modifying any files. Exits with code 1 if changes are pending, 0 if no changes.

#### Apply Updates

```bash
cd tools
node update_nodes.mjs
```

This fetches the latest RPC rankings and updates both `src/nodes.h` and `src/Certificates.h`:

**nodes.h updates:**
- Up to 3 URLs per chain (fastest first)
- Only HTTPS URLs (no websockets)
- No URLs with API key placeholders
- Deterministic output (same input = same output)

**Certificates.h updates:**
- Fetches TLS root CA certificate for each primary RPC URL
- Walks the certificate chain to extract the root CA
- Deduplicates identical roots (multiple hosts may share root CAs)
- Generates PEM-encoded certificates as const char* constants
- **Requires network access** - connects to each hostname via TLS

The generated sections are clearly marked with comments.

**To skip certificate fetching** (e.g., for testing nodes.h changes only):
```bash
node update_nodes.mjs --skip-certs
```

### Automated Updates (Windows)

The `auto-update.ps1` PowerShell script can be scheduled to run periodically (e.g., weekly) on a Windows PC.

```powershell
cd tools
.\auto-update.ps1
```

The script will:
1. Check for RPC node updates
2. If changes are found:
   - Create a new branch (`rpc-nodes-update-YYYYMMDD`)
   - Commit the changes
   - Push the branch
   - Create a pull request via GitHub CLI with a summary of changes

#### Scheduling on Windows

Create a scheduled task using Task Scheduler:

1. Open Task Scheduler
2. Create New Task
3. **Trigger**: Weekly (e.g., every Sunday at 3 AM)
4. **Action**: Start a program
   - Program: `powershell.exe`
   - Arguments: `-ExecutionPolicy Bypass -File "C:\path\to\Web3E\tools\auto-update.ps1"`
   - Start in: `C:\path\to\Web3E`

**Note**: Make sure GitHub CLI (`gh`) is installed and authenticated before scheduling:

```powershell
# Install GitHub CLI (if using winget)
winget install --id GitHub.cli

# Authenticate
gh auth login
```

### How It Works

1. **Feed Source**: The updater fetches RPC rankings from:
   ```
   https://pub-947f58bf7fb442f7a0d0686fcf757d76.r2.dev/rpc-rankings.json
   ```

2. **Selection Criteria**:
   - Only updates chains already present in `nodes.h`
   - Selects top 3 URLs based on response time (fastest first)
   - Filters out:
     - Non-HTTPS URLs
     - Websocket URLs (ws://, wss://)
     - URLs with API key placeholders (`${...}`, `YOUR_API_KEY`)
     - URLs flagged as rate-limited (future-proof)

3. **Certificate Fetching** (requires network access):
   - Connects to each primary RPC hostname via TLS (port 443)
   - Uses SNI (Server Name Indication) for modern TLS
   - Walks the certificate chain to find the root CA
   - Extracts root CA as PEM-encoded certificate
   - Deduplicates identical root CAs (e.g., many hosts use Google Trust Services)
   - Chains without successful cert fetch will use insecure mode (not recommended for mainnet)

4. **Output**:
   - **nodes.h**:
     - Primary URL: Available via `getNode(chainId)` (backwards compatible)
     - Alternate URLs: Available via `getNodeUrls(chainId, index)` where index is 0-2
     - All URLs stored as `PROGMEM` const char* for flash efficiency
   - **Certificates.h**:
     - Root CA certificates as PEM-encoded const char* constants
     - `getCertificate(chainId)` returns the root CA for the primary URL
     - If NULL, Web3::setupCert() uses insecure mode

5. **Determinism**:
   - URLs are sorted by chain ID
   - Certificates are sorted by fingerprint
   - Same feed + same TLS responses = same output
   - No changes = no diff

### API Changes in nodes.h

The updated `nodes.h` maintains backward compatibility while adding support for multiple URLs:

```cpp
// Existing API (unchanged)
const char* getNode(long long chainId);  // Returns primary URL

// New API for accessing alternates
const char* getNodeUrls(long long chainId, int index);  // index: 0-2
```

Example usage:

```cpp
// Get primary URL (backward compatible)
const char* primary = getNode(SEPOLIA_ID);

// Get all URLs for failover/rotation
for (int i = 0; i < 3; i++) {
    const char* url = getNodeUrls(SEPOLIA_ID, i);
    if (url != NULL) {
        // Try this URL
    }
}
```

### Maintenance

The feed is generated regularly with fresh response time measurements. Running the updater periodically ensures the library uses the fastest available public nodes.

**Recommended schedule**: Weekly or bi-weekly

The updater preserves URLs for chains not present in the feed, and does not automatically add new chains (maintainer must add chain IDs to both `chainIds.h` and the initial `nodes.h` manually).

### Infura Opt-In System

By default, Web3E uses **public RPCs only**. Infura support is **opt-in** via compile-time flags.

**Configuration Files:**
- `tools/infura-chains.json` - Maps chain IDs to Infura endpoints
- `tools/retired-chains.json` - Deny list for retired/reassigned chain IDs

**Generated Code:**
- `src/nodes.h` - Contains both public and Infura URL definitions with `#ifdef USING_INFURA` guards
- `src/Certificates.h` - Contains both public root CAs and Infura root CA (Amazon Root CA 1)
- `src/Web3.cpp` - Appends API key only when `USING_INFURA` is defined
- `src/Web3.h` - Guards `INFURA_API_KEY` with `#ifndef` and `#ifdef USING_INFURA`

**Usage:**
```ini
; platformio.ini - Default (public RPCs)
build_flags =
    -DARDUINO_LOOP_STACK_SIZE=16384

; platformio.ini - With Infura enabled
build_flags =
    -DARDUINO_LOOP_STACK_SIZE=16384
    -DUSING_INFURA
    -DINFURA_API_KEY="your-api-key-here"
```

**Behavior:**
- **Without `-DUSING_INFURA`**: Uses public RPCs from the feed, ignores Infura key in constructor
- **With `-DUSING_INFURA`**: Uses Infura as primary for supported chains, public RPCs as fallback
