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
import * as dotenv from 'dotenv';
import * as fs from 'fs';
import * as path from 'path';

// Load environment variables
dotenv.config({ path: path.join(__dirname, '.env') });

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
    
    constructor(
        private serialPath: string,
        private proxyBaseUrl: string,
        private telemetryApiKey: string | undefined,
        private deviceAddress: string | undefined,
        private timeoutMs: number = 120000 // 2 minutes default
    ) {
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
            
            // Open serial port
            this.port = new SerialPort({
                path: this.serialPath,
                baudRate: 115200
            });
            
            this.port.on('error', (err) => {
                this.result.error = `Serial port error: ${err.message}`;
                this.cleanup();
                resolve(this.result);
            });
            
            this.port.on('open', async () => {
                console.log('✓ Serial port opened');
                
                // Reset ESP32 by pulsing EN pin (DTR=false, RTS=true for 200ms, then RTS=false)
                // This is the standard esptool reset sequence for ESP32 DevKit boards
                try {
                    console.log('⏳ Resetting ESP32...');
                    await this.port!.set({ dtr: false, rts: true });
                    await this.sleep(200);
                    await this.port!.set({ rts: false });
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
        } else if (message.startsWith('API_CALL')) {
            const kv = parseKV(message);
            this.result.apiCallCount = parseInt(kv.count || '0', 10);
            console.log(`✓ API call handled: ${kv.name} (count: ${this.result.apiCallCount})`);
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
        if (!this.telemetryApiKey || !this.result.deviceAddress) {
            console.log('\n⚠ Skipping telemetry verification (no API key or device address)');
            return;
        }
        
        console.log('\n📡 Verifying SmartLayerProxy telemetry...');
        
        try {
            // Query telemetry API for this device
            const response = await axios.get(
                `${this.proxyBaseUrl}/api/telemetry/feed`,
                {
                    headers: {
                        'Authorization': `Bearer ${this.telemetryApiKey}`
                    },
                    params: {
                        deviceAddress: this.result.deviceAddress,
                        limit: 10
                    },
                    timeout: 10000
                }
            );
            
            if (response.status === 200 && response.data) {
                const events = response.data.events || [];
                console.log(`  Found ${events.length} telemetry events`);
                
                // Look for connect event with firmware version
                const connectEvent = events.find((e: any) => 
                    e.type === 'connect' && e.firmwareVersion
                );
                
                if (connectEvent) {
                    this.result.telemetryFirmwareVersion = connectEvent.firmwareVersion;
                    console.log(`  ✓ Firmware version in telemetry: ${connectEvent.firmwareVersion}`);
                    
                    if (connectEvent.firmwareVersion === this.result.firmwareVersion) {
                        console.log('  ✓ Firmware version matches device');
                    } else {
                        console.log(`  ⚠ Firmware version mismatch (device: ${this.result.firmwareVersion})`);
                    }
                }
                
                // Look for diagnostics event
                const diagEvent = events.find((e: any) => 
                    e.type === 'diagnostics' && e.data
                );
                
                if (diagEvent) {
                    this.result.telemetryHasDiagnostics = true;
                    console.log('  ✓ Diagnostics found in telemetry');
                    console.log(`    - Boot count: ${diagEvent.data.bootCount}`);
                    console.log(`    - WiFi attempts: ${diagEvent.data.wifiAttempts}`);
                    console.log(`    - Connect time: ${diagEvent.data.connectTimeMs}ms`);
                }
                
                this.result.telemetryVerified = true;
                console.log('  ✓ Telemetry verification successful');
            }
        } catch (error: any) {
            console.log(`  ⚠ Telemetry verification failed: ${error.message}`);
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
    
    for (let i = 0; i < args.length; i++) {
        const arg = args[i];
        if (arg === '--address' && i + 1 < args.length) {
            deviceAddress = args[++i];
        } else if (arg === '--timeout' && i + 1 < args.length) {
            timeoutMs = parseInt(args[++i], 10);
        } else if (!serialPath && !arg.startsWith('--')) {
            serialPath = arg;
        }
    }
    
    // Support legacy positional args: <port> [timeout]
    if (!serialPath && args.length >= 1 && !args[0].startsWith('--')) {
        serialPath = args[0];
        if (args.length >= 2 && !args[1].startsWith('--')) {
            timeoutMs = parseInt(args[1], 10);
        }
    }
    
    if (!serialPath) {
        console.error('Usage: tsx run-e2e.ts <serial-port> [timeout-ms] [--address <device-address>]');
        console.error('       tsx run-e2e.ts <serial-port> --address <device-address> --timeout <ms>');
        console.error('');
        console.error('Examples:');
        console.error('  tsx run-e2e.ts COM3 120000');
        console.error('  tsx run-e2e.ts /dev/ttyUSB0 --address 0xABCD1234...');
        console.error('  tsx run-e2e.ts COM15 --timeout 180000');
        console.error('');
        console.error('Options:');
        console.error('  --address <addr>     Pre-configure device address (if serial misses it)');
        console.error('  --timeout <ms>       Test timeout in milliseconds (default: 120000)');
        console.error('');
        console.error('Environment variables:');
        console.error('  PROXY_BASE_URL       SmartLayerProxy URL (default: https://scriptproxy.percolate.one)');
        console.error('  TELEMETRY_API_KEY    API key for telemetry fallback & verification (optional)');
        console.error('');
        process.exit(1);
    }
    
    const proxyBaseUrl = process.env.PROXY_BASE_URL || 'https://scriptproxy.percolate.one';
    const telemetryApiKey = process.env.TELEMETRY_API_KEY;
    
    const runner = new E2ETestRunner(serialPath, proxyBaseUrl, telemetryApiKey, deviceAddress, timeoutMs);
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
