#!/usr/bin/env pwsh
# Auto-update RPC nodes and create PR if changes detected
# This script is designed to be run on Windows as a scheduled task

param(
    [switch]$Force = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$toolsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $toolsDir

Write-Host "Web3E RPC Node Auto-Updater" -ForegroundColor Cyan
Write-Host "============================`n" -ForegroundColor Cyan

# Change to repository root
Set-Location $repoRoot

# Check if in git repository
if (-not (Test-Path ".git")) {
    Write-Error "Not in a git repository root. Expected to be in Web3E root."
    exit 1
}

# Ensure we're on master branch
$currentBranch = git rev-parse --abbrev-ref HEAD
if ($currentBranch -ne "master") {
    Write-Host "Switching to master branch..." -ForegroundColor Yellow
    git checkout master
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to checkout master branch"
        exit 1
    }
}

# Pull latest changes
Write-Host "Pulling latest changes from origin/master..." -ForegroundColor Yellow
git pull origin master
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to pull latest changes"
    exit 1
}

# Run the updater in dry-run mode first
Write-Host "`nChecking for RPC node updates..." -ForegroundColor Yellow
Set-Location $toolsDir

# Ensure dependencies are installed
if (-not (Test-Path "node_modules")) {
    Write-Host "Installing Node.js dependencies..." -ForegroundColor Yellow
    npm install --silent
}

# Run in check mode to see if there are changes
$checkOutput = node update_nodes.mjs --check 2>&1
$hasChanges = $LASTEXITCODE -ne 0

if (-not $hasChanges -and -not $Force) {
    Write-Host "`nNo RPC node updates needed. Exiting." -ForegroundColor Green
    exit 0
}

if ($Force) {
    Write-Host "`nForce flag set, proceeding with update even if no changes detected..." -ForegroundColor Yellow
}

# Run the actual update
Write-Host "`nApplying RPC node updates..." -ForegroundColor Yellow
$updateOutput = node update_nodes.mjs 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to update nodes"
    exit 1
}

# Go back to repo root
Set-Location $repoRoot

# Check if nodes.h was actually changed
$gitStatus = git status --porcelain src/nodes.h
if (-not $gitStatus -and -not $Force) {
    Write-Host "`nNo changes to nodes.h after update. Exiting." -ForegroundColor Green
    exit 0
}

# Parse the update output to extract changes for PR body
$changesSection = $checkOutput | Select-String -Pattern "Changes detected:" -Context 0, 100 | Out-String
$prBodyChanges = @()

# Extract chain changes from output
$lines = $checkOutput -split "`n"
$inChangesSection = $false
for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i].Trim()
    
    if ($line -match "^Chain \d+ \(([^)]+)\):") {
        $chainName = $matches[1]
        $i++
        
        $oldUrl = ""
        $newUrl = ""
        $responseTime = ""
        
        while ($i -lt $lines.Count -and $lines[$i].Trim() -match "(Old|New):\s+(.+)") {
            $field = $matches[1]
            $value = $matches[2].Trim()
            
            if ($field -eq "Old") {
                $oldUrl = $value
            }
            elseif ($field -eq "New") {
                # Extract URL and response time
                if ($value -match "^(.+?)\s+\(([0-9.]+)ms\)$") {
                    $newUrl = $matches[1]
                    $responseTime = $matches[2]
                }
                else {
                    $newUrl = $value
                }
            }
            
            $i++
        }
        
        if ($oldUrl -and $newUrl -and $oldUrl -ne $newUrl) {
            $prBodyChanges += "- **$chainName**: ``$oldUrl`` → ``$newUrl`` ($responseTime ms)"
        }
    }
}

# Create a new branch
$timestamp = Get-Date -Format "yyyyMMdd"
$branchName = "rpc-nodes-update-$timestamp"

Write-Host "`nCreating branch: $branchName" -ForegroundColor Yellow
git checkout -b $branchName
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to create branch"
    exit 1
}

# Stage and commit changes
Write-Host "Staging and committing changes..." -ForegroundColor Yellow
git add src/nodes.h
git commit -m "Update RPC node URLs from rankings feed

Updated RPC endpoints to use the fastest available nodes based on
response time rankings from the build-time feed.

Generated at: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to commit changes"
    exit 1
}

# Push the branch
Write-Host "Pushing branch to origin..." -ForegroundColor Yellow
git push -u origin $branchName
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to push branch"
    exit 1
}

# Create PR body
$prBody = @"
This PR updates the RPC node URLs in ``src/nodes.h`` based on the latest performance rankings from the build-time feed.

## Primary URL Changes

$($prBodyChanges -join "`n")

## Summary

- Updated RPC endpoints to use the fastest available public nodes
- Up to 3 URLs per chain for future failover/rotation support
- Changes are deterministic and based on response time measurements
- Generated from: https://pub-947f58bf7fb442f7a0d0686fcf757d76.r2.dev/rpc-rankings.json

The updater script automatically selects the top 3 HTTPS URLs for each chain (excluding websockets and URLs with API key placeholders).

---
*This PR was automatically generated by tools/auto-update.ps1*
"@

# Create PR using gh CLI
Write-Host "`nCreating pull request..." -ForegroundColor Yellow
gh pr create `
    --title "Update RPC node URLs - $timestamp" `
    --body $prBody `
    --base master `
    --head $branchName

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to create PR. You may need to install GitHub CLI (gh) and authenticate."
    Write-Host "`nBranch $branchName has been pushed. You can manually create a PR at:" -ForegroundColor Yellow
    Write-Host "https://github.com/AlphaWallet/Web3E/compare/$branchName" -ForegroundColor Cyan
    exit 1
}

Write-Host "`n✓ Successfully created pull request!" -ForegroundColor Green
Write-Host "Branch: $branchName" -ForegroundColor Cyan

# Return to master
git checkout master
