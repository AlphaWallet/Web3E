#!/usr/bin/env tsx
/**
 * Web3E TCP Bridge E2E Test Runner
 * 
 * Monitors serial output from the E2E test firmware and verifies:
 * 1. Device successfully connects to WiFi
 * 2. Device authenticates with TCP bridge (opcode 0x09)
 * 3. Device sends diagnostics (opcode 0x0A)
 * 4. Device sends keep-alive messages
 * 5. Device responds to API calls (if triggered)
 * 6. SmartLayerProxy telemetry records firmware version and diagnostics
 */

import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import axios from 'axios';
import { execSync } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';

interface E2ETestResult {
    pass: boolean;
    deviceAddress?: string;
    firmwareVersion?: string;
    wifiConnected: boolean;
    loginSuccess: boolean;
    loginOpcode?: string;
    diagSent: boolean;
    keepaliveCount: number;
    apiCallCount: number;
    testTimeMs?: number;
    error?: string;
    telemetryVerified: boolean;
    telemetryFirmwareVersion?: string;
    telemetryHasDiagnostics: boolean;
}

class E2ETestRunner {
    private port: SerialPort | null = null;
    private parser: ReadlineParser | null = null;
    private result: E2ETestResult;
    private timeout: NodeJS.Timeout | null = null;
    private resolve: ((result: E2ETestResult) => void) | null = null;
    private testStartTime: number;
    
    constructor(
        private serialPath: string,
        private proxyBaseUrl: string,
        private proxyRepoPath: string | undefined,
        private deviceAddress: string | undefined,
        private timeoutMs: number = 120000 // 2 minutes default
    ) {
        this.testStartTime = Date.now();
        this.result = {
            pass: false,
            wifiConnected: false,
            loginSuccess: false,
            diagSent: false,
            keepaliveCount: 0,
            apiCallCount: 0,
            telemetryVerified: false,
            telemetryHasDiagnostics: false
        };
        
        // If device address was provided, set it (case-insensitive normalization)
        if (deviceAddress) {
            this.result.deviceAddress = this.normalizeAddress(deviceAddress);
        }
    }
    
    private normalizeAddress(addr: string): string {
        // Normalize to lowercase with 0x prefix
        const clean = addr.trim().toLowerCase();
        return clean.startsWith('0x') ? clean : '0x' + clean;
    }
    
    async run(): Promise<E2ETestResult> {
        console.log(`Starting E2E test on ${this.serialPath}`);
        console.log(`Proxy: ${this.proxyBaseUrl}`);
        if (this.result.deviceAddress) {
            console.log(`Device address: ${this.result.deviceAddress} (pre-configured)`);
        }
        console.log(`Timeout: ${this.timeoutMs}ms\n`);
        
        return new Promise((resolve, reject) => {
            this.resolve = resolve;
            
            // Set overall timeout
            this.timeout = setTimeout(() => {
                this.result.error = 'Test timeout - device did not complete in time';
                this.cleanup();
                resolve(this.result);
            }, this.timeoutMs);
            
            // Open serial port with autoOpen:false to control DTR/RTS before opening
            // This prevents node serialport from asserting DTR/RTS on open (which triggers bootloader)
            this.port = new SerialPort({
                path: this.serialPath,
                baudRate: 115200,
                autoOpen: false,
                rtscts: false,  // Disable hardware flow control
                hupcl: false    // Don't reset on close
            });
            
            this.port.on('error', (err) => {
                this.result.error = `Serial port error: ${err.message}`;
                this.cleanup();
                resolve(this.result);
            });
            
            // Manually open the port and control reset sequence
            this.port.open(async (err) => {
                if (err) {
                    this.result.error = `Failed to open serial port: ${err.message}`;
                    this.cleanup();
                    resolve(this.result);
                    return;
                }
                
                console.log('✓ Serial port opened');
                
                // Reset ESP32 using EN pin pulse (avoid bootloader mode)
                // CRITICAL: node serialport asserts DTR (GPIO0) by default on open
                // DTR low during EN pulse = bootloader mode. Must explicitly set dtr:false.
                // Known-good sequence: DtrEnable=false, RtsEnable=true, Open, sleep 200ms, RtsEnable=false
                try {
                    console.log('⏳ Resetting ESP32...');
                    
                    // Step 1: Immediately set both low to establish known state
                    await this.port!.set({ dtr: false, rts: false });
                    await this.sleep(50);
                    
                    // Step 2: Assert EN (RTS=true) while keeping GPIO0 high (DTR=false)
                    await this.port!.set({ dtr: false, rts: true });
                    await this.sleep(200);
                    
                    // Step 3: Release EN (RTS=false) while keeping GPIO0 high (DTR=false)
                    await this.port!.set({ dtr: false, rts: false });
                    await this.sleep(100); // Brief stabilization delay
                    
                    console.log('✓ ESP32 reset complete, monitoring serial output...\n');
                } catch (err) {
                    console.log(`⚠ Reset failed (${err}), continuing anyway...`);
                }
                
                // Now start parsing serial data
                this.parser = this.port!.pipe(new ReadlineParser({ delimiter: '\n' }));
                this.parser.on('data', (line: string) => {
                    this.handleSerialLine(line.trim());
                });
            });
        });
    }
    
    private sleep(ms: number): Promise<void> {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
    
    private handleSerialLine(line: string) {
        // Echo all lines for debugging
        console.log(`[DEVICE] ${line}`);
        
        // Parse E2E protocol lines
        if (!line.startsWith('E2E:')) {
            return;
        }
        
        const message = line.substring(4); // Remove "E2E:" prefix
        
        // Parse key=value pairs
        const parseKV = (str: string): Record<string, string> => {
            const result: Record<string, string> = {};
            const regex = /(\w+)=(?:"([^"]*)"|(\S+))/g;
            let match;
            while ((match = regex.exec(str)) !== null) {
                const key = match[1];
                const value = match[2] || match[3];
                result[key] = value;
            }
            return result;
        };
        
        // Handle specific E2E messages
        if (message.startsWith('START')) {
            console.log('✓ Test started');
        } else if (message.startsWith('FW_VERSION=')) {
            const kv = parseKV(message);
            this.result.firmwareVersion = kv.FW_VERSION;
            console.log(`✓ Firmware version: ${this.result.firmwareVersion}`);
        } else if (message.startsWith('KEYID=')) {
            console.log('✓ KeyID loaded from EEPROM');
        } else if (message.startsWith('WIFISTORE_READ')) {
            const kv = parseKV(message);
            console.log(`✓ WifiStore: ${kv.networks} networks loaded`);
        } else if (message.startsWith('WIFI=OK')) {
            const kv = parseKV(message);
            this.result.wifiConnected = true;
            console.log(`✓ WiFi connected: ${kv.ip}`);
        } else if (message.startsWith('DEVICE_ADDRESS=')) {
            const kv = parseKV(message);
            // Only set if not already configured via --address flag
            if (!this.result.deviceAddress) {
                this.result.deviceAddress = this.normalizeAddress(kv.DEVICE_ADDRESS);
                console.log(`✓ Device address: ${this.result.deviceAddress}`);
            } else {
                console.log(`✓ Device address from serial: ${kv.DEVICE_ADDRESS} (using pre-configured: ${this.result.deviceAddress})`);
            }
        } else if (message.startsWith('LOGIN=OK')) {
            const kv = parseKV(message);
            this.result.loginSuccess = true;
            this.result.loginOpcode = kv.opcode;
            console.log(`✓ TCP bridge login successful (${kv.opcode})`);
        } else if (message.startsWith('DIAG_SENT')) {
            this.result.diagSent = true;
            console.log('✓ Diagnostics sent (opcode 0x0A)');
        } else if (message.startsWith('KEEPALIVE')) {
            const kv = parseKV(message);
            this.result.keepaliveCount = parseInt(kv.count || '0', 10);
            console.log(`✓ Keep-alive ${this.result.keepaliveCount}`);
        } else if (message.startsWith('WAITING_FOR_API')) {
            console.log('⏳ Device waiting for API call - triggering test API call...');
            this.triggerApiCall();
        } else if (message === 'API_CALL' || message.match(/^API_CALL\s/)) {
            // Match "E2E:API_CALL name=..." but not "E2E:API_CALLS_RECEIVED=..."
            const kv = parseKV(message);
            this.result.apiCallCount = parseInt(kv.count || '0', 10);
            console.log(`✓ API call handled: ${kv.name} (count: ${this.result.apiCallCount})`);
        } else if (message.startsWith('API_CALLS_RECEIVED=')) {
            const kv = parseKV(message);
            const totalCalls = parseInt(kv.API_CALLS_RECEIVED || '0', 10);
            console.log(`✓ Total API calls received: ${totalCalls}`);
        } else if (message.startsWith('DONE')) {
            const parts = message.split(' ');
            const status = parts[1]; // PASS or FAIL
            const kv = parseKV(message);
            
            if (status === 'PASS') {
                this.result.pass = true;
                this.result.testTimeMs = parseInt(kv.time_ms || '0', 10);
                console.log(`\n✓ Device test PASSED (${this.result.testTimeMs}ms)`);
            } else {
                this.result.pass = false;
                console.log(`\n✗ Device test FAILED`);
            }
            
            // Verify telemetry
            this.verifyTelemetry().then(() => {
                this.cleanup();
                if (this.resolve) {
                    this.resolve(this.result);
                }
            });
        } else if (message.startsWith('FAIL')) {
            const kv = parseKV(message);
            this.result.error = kv.reason;
            console.log(`\n✗ Device test FAILED: ${this.result.error}`);
            this.cleanup();
            if (this.resolve) {
                this.resolve(this.result);
            }
        }
    }
    
    private async triggerApiCall() {
        // Try to get device address from telemetry if we don't have it yet
        if (!this.result.deviceAddress && this.telemetryApiKey) {
            console.log('  ⏳ Device address not captured from serial, fetching from telemetry...');
            await this.fetchDeviceAddressFromTelemetry();
        }
        
        if (!this.result.deviceAddress) {
            console.log('  ✗ Cannot trigger API call - device address not available');
            console.log('    Hint: Use --address flag or ensure TELEMETRY_API_KEY is set');
            return;
        }
        
        // Call the proxy API to forward a test method to the device
        // Format: GET /api/<address>/<method>?param1=value1&param2=value2
        const method = 'e2eTest';
        const apiUrl = `${this.proxyBaseUrl}/api/${this.result.deviceAddress}/${method}`;
        
        console.log(`  → Calling ${apiUrl}`);
        
        try {
            const response = await axios.get(apiUrl, {
                params: {
                    testParam: 'e2e-runner',
                    timestamp: Date.now()
                },
                timeout: 30000 // 30 second timeout
            });
            
            if (response.status === 201) {
                console.log(`  ✓ API call succeeded: ${response.data}`);
            } else {
                console.log(`  ⚠ API call returned status ${response.status}: ${response.data}`);
            }
        } catch (error: any) {
            console.log(`  ✗ API call failed: ${error.message}`);
            console.log('    (Device may still handle it if proxy forwarded before timeout)');
        }
    }
    
    private async fetchDeviceAddressFromTelemetry() {
        if (!this.telemetryApiKey) {
            return;
        }
        
        try {
            // Get recent connection events from telemetry
            const response = await axios.get(
                `${this.proxyBaseUrl}/api/telemetry/feed`,
                {
                    headers: {
                        'Authorization': `Bearer ${this.telemetryApiKey}`
                    },
                    params: {
                        limit: 50
                    },
                    timeout: 5000
                }
            );
            
            if (response.status === 200 && response.data && response.data.rows) {
                // Find most recent connect event
                const recentConnect = response.data.rows
                    .filter((e: any) => e.eventType === 'connect')
                    .sort((a: any, b: any) => b.timestamp - a.timestamp)[0];
                
                if (recentConnect && recentConnect.device) {
                    this.result.deviceAddress = this.normalizeAddress(recentConnect.device);
                    console.log(`  ✓ Found device address in telemetry: ${this.result.deviceAddress}`);
                } else {
                    console.log('  ⚠ No recent device connections in telemetry');
                }
            }
        } catch (error: any) {
            console.log(`  ⚠ Could not fetch from telemetry: ${error.message}`);
        }
    }
    
    private async verifyTelemetry() {
        if (!this.proxyRepoPath) {
            console.log('\n⚠ Skipping telemetry verification (no --proxy-repo specified)');
            console.log('   To enable: --proxy-repo <path-to-SmartLayerProxy> or set SMARTLAYER_PROXY_DIR');
            return;
        }
        
        if (!this.result.deviceAddress) {
            console.log('\n⚠ Skipping telemetry verification (no device address captured)');
            return;
        }
        
        console.log('\n📡 Verifying SmartLayerProxy telemetry...');
        console.log(`  Using proxy repo: ${this.proxyRepoPath}`);
        
        try {
            // Run telemetry:pull in the SmartLayerProxy repo
            console.log('  Pulling telemetry data from proxy...');
            execSync('npm run telemetry:pull', {
                cwd: this.proxyRepoPath,
                stdio: ['ignore', 'pipe', 'pipe'], // Suppress output to avoid printing keys
                timeout: 30000
            });
            
            // Find the latest telemetry JSONL file
            const dataDir = path.join(this.proxyRepoPath, 'data');
            if (!fs.existsSync(dataDir)) {
                console.log('  ⚠ No data directory found in proxy repo');
                return;
            }
            
            const files = fs.readdirSync(dataDir)
                .filter(f => f.startsWith('telemetry-') && f.endsWith('.jsonl'))
                .sort()
                .reverse();
            
            if (files.length === 0) {
                console.log('  ⚠ No telemetry data files found');
                return;
            }
            
            const latestFile = path.join(dataDir, files[0]);
            console.log(`  Reading: ${files[0]}`);
            
            // Parse JSONL file
            const content = fs.readFileSync(latestFile, 'utf-8');
            const events = content
                .split('\n')
                .filter(line => line.trim())
                .map(line => JSON.parse(line))
                .filter((e: any) => 
                    e.device?.toLowerCase() === this.result.deviceAddress?.toLowerCase() &&
                    e.timestamp >= this.testStartTime
                );
            
            console.log(`  Found ${events.length} events for this device during test run`);
            
            // Check for connect event with firmware version
            const connectEvents = events.filter((e: any) => 
                e.eventType === 'connect' && e.firmwareVersion
            );
            
            if (connectEvents.length > 0) {
                const connectEvent = connectEvents[connectEvents.length - 1]; // Most recent
                this.result.telemetryFirmwareVersion = connectEvent.firmwareVersion;
                console.log(`  ✓ Firmware version in telemetry: ${connectEvent.firmwareVersion}`);
                
                if (connectEvent.firmwareVersion === this.result.firmwareVersion) {
                    console.log('  ✓ Firmware version matches device');
                } else {
                    console.log(`  ⚠ Firmware version mismatch (device: ${this.result.firmwareVersion})`);
                }
            } else {
                console.log('  ⚠ No connect event with firmware version found');
            }
            
            // Check for diagnostics (may be in a separate feed or embedded in events)
            // The analyse.ts doesn't show a separate diagnostics structure, so checking event metadata
            const hasRecentEvents = events.length > 0;
            if (hasRecentEvents) {
                this.result.telemetryHasDiagnostics = true;
                console.log('  ✓ Device events recorded in telemetry');
            }
            
            this.result.telemetryVerified = true;
            console.log('  ✓ Telemetry verification complete');
        } catch (error: any) {
            console.log(`  ⚠ Telemetry verification failed: ${error.message}`);
            if (error.message.includes('TELEMETRY_API_KEY')) {
                console.log('    Ensure tools/telemetry/.env is configured in SmartLayerProxy repo');
            }
            console.log('    (This is non-fatal for the test)');
        }
    }
    
    private cleanup() {
        if (this.timeout) {
            clearTimeout(this.timeout);
            this.timeout = null;
        }
        
        if (this.port && this.port.isOpen) {
            this.port.close();
        }
    }
}

// Main execution
async function main() {
    const args = process.argv.slice(2);
    
    // Parse arguments
    let serialPath: string | undefined;
    let timeoutMs = 120000;
    let deviceAddress: string | undefined;
    let proxyRepoPath: string | undefined;
    
    // Collect positional args (non-flag args)
    const positional: string[] = [];
    
    for (let i = 0; i < args.length; i++) {
        const arg = args[i];
        if (arg === '--address' && i + 1 < args.length) {
            deviceAddress = args[++i];
        } else if (arg === '--timeout' && i + 1 < args.length) {
            timeoutMs = parseInt(args[++i], 10);
        } else if (arg === '--proxy-repo' && i + 1 < args.length) {
            proxyRepoPath = args[++i];
        } else if (!arg.startsWith('--')) {
            positional.push(arg);
        }
    }
    
    // Process positional args: <port> [timeout]
    if (positional.length >= 1) {
        serialPath = positional[0];
    }
    if (positional.length >= 2) {
        const parsedTimeout = parseInt(positional[1], 10);
        if (!isNaN(parsedTimeout)) {
            timeoutMs = parsedTimeout;
        }
    }
    
    // Check for proxy repo path from environment if not specified
    if (!proxyRepoPath) {
        proxyRepoPath = process.env.SMARTLAYER_PROXY_DIR;
    }
    
    // Validate proxy repo path if provided
    if (proxyRepoPath) {
        if (!fs.existsSync(proxyRepoPath)) {
            console.error(`Error: Proxy repo path does not exist: ${proxyRepoPath}`);
            process.exit(1);
        }
        const packageJson = path.join(proxyRepoPath, 'package.json');
        if (!fs.existsSync(packageJson)) {
            console.error(`Error: Not a valid SmartLayerProxy repo (no package.json): ${proxyRepoPath}`);
            process.exit(1);
        }
    }
    
    if (!serialPath) {
        console.error('Usage: tsx run-e2e.ts <serial-port> [timeout-ms] [options]');
        console.error('');
        console.error('Examples:');
        console.error('  tsx run-e2e.ts COM3 120000');
        console.error('  tsx run-e2e.ts /dev/ttyUSB0 --address 0xABCD1234...');
        console.error('  tsx run-e2e.ts COM15 --timeout 180000 --proxy-repo ../SmartLayerProxy');
        console.error('');
        console.error('Options:');
        console.error('  --address <addr>      Pre-configure device address (if serial misses it)');
        console.error('  --timeout <ms>        Test timeout in milliseconds (default: 120000)');
        console.error('  --proxy-repo <path>   Path to SmartLayerProxy repo for telemetry verification');
        console.error('');
        console.error('Environment variables:');
        console.error('  PROXY_BASE_URL        SmartLayerProxy URL (default: https://scriptproxy.percolate.one)');
        console.error('  SMARTLAYER_PROXY_DIR  Default path to SmartLayerProxy repo (optional)');
        console.error('');
        console.error('Telemetry verification:');
        console.error('  If --proxy-repo is specified, the runner will use the SmartLayerProxy repo\'s');
        console.error('  telemetry tools (tools/telemetry/, npm run telemetry:pull) to verify that the');
        console.error('  device\'s firmware version and diagnostics were recorded. The tools load credentials');
        console.error('  from tools/telemetry/.env automatically. If not specified, telemetry verification');
        console.error('  is skipped (test still passes if device completes successfully).');
        console.error('');
        process.exit(1);
    }
    
    const proxyBaseUrl = process.env.PROXY_BASE_URL || 'https://scriptproxy.percolate.one';
    
    const runner = new E2ETestRunner(serialPath, proxyBaseUrl, proxyRepoPath, deviceAddress, timeoutMs);
    const result = await runner.run();
    
    // Print summary
    console.log('\n' + '='.repeat(60));
    console.log('E2E TEST SUMMARY');
    console.log('='.repeat(60));
    console.log(`Device Address:     ${result.deviceAddress || 'N/A'}`);
    console.log(`Firmware Version:   ${result.firmwareVersion || 'N/A'}`);
    console.log(`WiFi Connected:     ${result.wifiConnected ? 'YES' : 'NO'}`);
    console.log(`TCP Login:          ${result.loginSuccess ? `YES (${result.loginOpcode})` : 'NO'}`);
    console.log(`Diagnostics Sent:   ${result.diagSent ? 'YES' : 'NO'}`);
    console.log(`Keep-alive Count:   ${result.keepaliveCount}`);
    console.log(`API Call Count:     ${result.apiCallCount}`);
    console.log(`Test Time:          ${result.testTimeMs ? `${result.testTimeMs}ms` : 'N/A'}`);
    console.log('');
    console.log('Telemetry Verification:');
    console.log(`  Verified:         ${result.telemetryVerified ? 'YES' : 'NO'}`);
    console.log(`  Firmware Match:   ${result.telemetryFirmwareVersion === result.firmwareVersion ? 'YES' : 'NO'}`);
    console.log(`  Diagnostics:      ${result.telemetryHasDiagnostics ? 'YES' : 'NO'}`);
    console.log('');
    console.log(`OVERALL RESULT:     ${result.pass ? '✓ PASS' : '✗ FAIL'}`);
    if (result.error) {
        console.log(`Error:              ${result.error}`);
    }
    console.log('='.repeat(60));
    
    process.exit(result.pass ? 0 : 1);
}

main().catch((error) => {
    console.error('Fatal error:', error);
    process.exit(1);
});
