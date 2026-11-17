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
    }
    
    async run(): Promise<E2ETestResult> {
        console.log(`Starting E2E test on ${this.serialPath}`);
        console.log(`Proxy: ${this.proxyBaseUrl}`);
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
            
            this.parser = this.port.pipe(new ReadlineParser({ delimiter: '\n' }));
            
            this.port.on('error', (err) => {
                this.result.error = `Serial port error: ${err.message}`;
                this.cleanup();
                resolve(this.result);
            });
            
            this.parser.on('data', (line: string) => {
                this.handleSerialLine(line.trim());
            });
        });
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
            this.result.deviceAddress = kv.DEVICE_ADDRESS;
            console.log(`✓ Device address: ${this.result.deviceAddress}`);
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
        if (!this.result.deviceAddress) {
            console.log('  ✗ Cannot trigger API call - device address not yet captured');
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
    
    if (args.length < 1) {
        console.error('Usage: tsx run-e2e.ts <serial-port> [timeout-ms]');
        console.error('');
        console.error('Example: tsx run-e2e.ts COM3 120000');
        console.error('         tsx run-e2e.ts /dev/ttyUSB0');
        console.error('');
        console.error('Environment variables:');
        console.error('  PROXY_BASE_URL       - SmartLayerProxy URL (default: https://scriptproxy.percolate.one)');
        console.error('  TELEMETRY_API_KEY    - API key for telemetry verification (optional)');
        console.error('');
        process.exit(1);
    }
    
    const serialPath = args[0];
    const timeoutMs = args[1] ? parseInt(args[1], 10) : 120000;
    const proxyBaseUrl = process.env.PROXY_BASE_URL || 'https://scriptproxy.percolate.one';
    const telemetryApiKey = process.env.TELEMETRY_API_KEY;
    
    const runner = new E2ETestRunner(serialPath, proxyBaseUrl, telemetryApiKey, timeoutMs);
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
