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

This fetches the latest RPC rankings and updates `src/nodes.h` with:
- Up to 3 URLs per chain (fastest first)
- Only HTTPS URLs (no websockets)
- No URLs with API key placeholders
- Deterministic output (same input = same output)

The generated section in `nodes.h` is clearly marked with comments.

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

3. **Output**:
   - Primary URL: Available via `getNode(chainId)` (backwards compatible)
   - Alternate URLs: Available via `getNodeUrls(chainId, index)` where index is 0-2
   - All URLs stored as `PROGMEM` const char* for flash efficiency

4. **Determinism**:
   - URLs are sorted by chain ID
   - Same feed = same output
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
