# Web3E TCP Bridge End-to-End Hardware Test

Comprehensive hardware test for the TCP bridge protocol including firmware version reporting (opcode 0x09) and WiFi diagnostics (opcode 0x0A).

## Requirements

### Hardware
- ESP32 development board
- USB cable for programming and serial monitoring

### Pre-Provisioning
The device **must** be pre-provisioned using [Web3E-Setup-Firmware](https://github.com/JamesSmartCell/Web3E-Setup-Firmware) before running this test.

Pre-provisioning writes:
1. **KeyID** at EEPROM offset 0-32: `[0]=64 marker, [1-32]=32-byte private key`
2. **WiFi networks** at EEPROM offset 33+: WifiStore format with magic, version, networks, terminator, and CRC32

**This test firmware NEVER creates or overwrites keys - it only reads pre-provisioned data.**

### Software
- PlatformIO CLI
- Node.js 18+ and npm (for test runner)
- Git (for firmware version tagging)

## Project Structure

```
test/hardware/proxy_e2e/
├── src/
│   └── main.cpp          # E2E test firmware
├── platformio.ini        # PlatformIO project configuration
├── git_version.py        # Build script to inject git SHA
├── package.json          # Node.js dependencies
├── run-e2e.ts           # TypeScript test runner
├── .env.example         # Environment variable template
└── README.md            # This file
```

## Setup

### 1. Device Pre-Provisioning

Flash the device with Web3E-Setup-Firmware first:

```bash
# Clone and build Web3E-Setup-Firmware
git clone https://github.com/JamesSmartCell/Web3E-Setup-Firmware
cd Web3E-Setup-Firmware
pio run -t upload
```

Follow the setup firmware instructions to:
1. Generate a device private key (stored at EEPROM offset 0)
2. Configure WiFi networks (stored at EEPROM offset 33)

After provisioning, note the device's Ethereum address (displayed on serial).

### 2. Build E2E Test Firmware

```bash
cd test/hardware/proxy_e2e

# Build firmware
pio run

# Or build and upload in one step
pio run -t upload -upload-port COM3  # Windows
pio run -t upload --upload-port /dev/ttyUSB0  # Linux
pio run -t upload --upload-port /dev/cu.usbserial-*  # macOS
```

The firmware version is automatically set to `e2e-<git-short-sha>` (e.g., `e2e-a1b2c3d`).

### 3. Install Test Runner Dependencies

```bash
npm install
```

### 4. Configure Environment (Optional)

The test runner can load environment variables from `.env` for configuration.

**Create `.env` file (gitignored):**

```bash
cp .env.example .env
```

**Edit `.env`:**

```bash
# SmartLayerProxy base URL (default: https://scriptproxy.percolate.one)
PROXY_BASE_URL=https://scriptproxy.percolate.one

# Telemetry API key for verification (optional but recommended)
# Obtain from SmartLayerProxy admin
TELEMETRY_API_KEY=your_api_key_here
```

**Telemetry Verification:**

When `TELEMETRY_API_KEY` is configured, the runner will:
- Verify the device's firmware version matches in telemetry
- Verify WiFi diagnostics (opcode 0x0A) were recorded
- Provide fallback device address lookup if serial output missed

Without the API key, the test will skip telemetry verification but still pass if the device completes the protocol successfully.

**Custom Environment Files:**

Use `--env-file` to load from a different location:

```bash
npx tsx run-e2e.ts COM3 --env-file .env.production
```

## Running the Test

### Option 1: Automatic Test (Recommended)

Build, flash, and run the test in one command:

```bash
# Flash firmware and run test
pio run -t upload --upload-port COM3 && npm test COM3
```

### Option 2: Manual Steps

**Step 1: Flash firmware**

```bash
pio run -t upload --upload-port COM3
```

**Step 2: Run test runner**

```bash
npm test COM3

# Or directly with tsx:
npx tsx run-e2e.ts COM3

# With custom timeout (default 120 seconds):
npx tsx run-e2e.ts COM3 180000  # 3 minutes

# With pre-configured device address (if serial output is missed):
npx tsx run-e2e.ts COM3 --address 0xABCD1234...

# With custom environment file:
npx tsx run-e2e.ts COM3 --env-file .env.production

# All options:
npx tsx run-e2e.ts COM15 --timeout 180000 --address 0x123... --env-file .env.staging
```

**CLI Options:**

- `--address <addr>`: Pre-configure device address (helps if serial misses it)
- `--timeout <ms>`: Test timeout in milliseconds (default: 120000)
- `--env-file <path>`: Load environment variables from custom file (default: `.env` in this folder if present)

**Note:** The runner automatically resets the ESP32 when it opens the serial port (DTR/RTS pulse). This ensures it captures all serial output from boot. If the device address is missed, use `--address` flag or set `TELEMETRY_API_KEY` for automatic fallback.

## Test Protocol

The test firmware and runner follow this sequence:

```
1. E2E:START
2. E2E:FW_VERSION=e2e-abc123
3. E2E:EEPROM_SIZE=1024
4. E2E:KEYID=OK found=true
5. E2E:WIFISTORE_READ networks=2
6. E2E:WIFI=OK ip="192.168.1.100"
7. E2E:DEVICE_ADDRESS="0x..."
8. E2E:LOGIN=OK opcode=0x09
9. E2E:DIAG_SENT opcode=0x0A
10. E2E:KEEPALIVE count=1
11. E2E:KEEPALIVE count=2
12. E2E:KEEPALIVE count=3
13. E2E:WAITING_FOR_API
    → Runner calls: GET https://scriptproxy.percolate.one/api/<device_address>/e2eTest
    → Proxy forwards opcode 0x04 to device
    → Device receives API call and responds with opcode 0x08
14. E2E:API_CALL name="e2eTest" count=1
15. E2E:DONE PASS time_ms=45000
```

**Note:** The test runner actively triggers an API call when it sees `WAITING_FOR_API`. The device will **fail** if no API call is received within 60 seconds, as this tests the complete end-to-end flow through the public proxy.

## Serial Line Format

All test lines start with `E2E:` prefix and use key=value pairs:

```
E2E:TEST=STATUS key1=value1 key2="quoted value"
```

**Key Messages:**

| Line | Description |
|------|-------------|
| `E2E:START` | Test started |
| `E2E:FW_VERSION=<version>` | Firmware version |
| `E2E:EEPROM_SIZE=<size>` | EEPROM size opened (bytes) |
| `E2E:KEYID=OK` | KeyID loaded from EEPROM |
| `E2E:WIFISTORE_READ networks=N` | N networks loaded |
| `E2E:WIFI=OK ip="..."` | WiFi connected |
| `E2E:DEVICE_ADDRESS="0x..."` | Device Ethereum address |
| `E2E:LOGIN=OK opcode=0x09` | TCP bridge authenticated |
| `E2E:DIAG_SENT opcode=0x0A` | Diagnostics sent |
| `E2E:KEEPALIVE count=N` | Keep-alive message N |
| `E2E:WAITING_FOR_API` | Waiting for API call from runner |
| `E2E:API_CALL name="..." count=N` | API call N handled |
| `E2E:API_CALLS_RECEIVED=N` | Total API calls received |
| `E2E:DONE PASS time_ms=T` | Test passed in T ms |
| `E2E:DONE FAIL` | Test failed |
| `E2E:FAIL reason="..."` | Failure reason |

## Test Runner Output

The TypeScript runner monitors serial output and verifies telemetry:

```
Starting E2E test on COM3
Proxy: https://scriptproxy.percolate.one
Timeout: 120000ms

[DEVICE] E2E:START
✓ Test started
[DEVICE] E2E:FW_VERSION=e2e-a1b2c3d
✓ Firmware version: e2e-a1b2c3d
[DEVICE] E2E:WIFI=OK ip="192.168.1.100"
✓ WiFi connected: 192.168.1.100
[DEVICE] E2E:DEVICE_ADDRESS="0x1234..."
✓ Device address: 0x1234...
[DEVICE] E2E:LOGIN=OK opcode=0x09
✓ TCP bridge login successful (0x09)
[DEVICE] E2E:DIAG_SENT
✓ Diagnostics sent (opcode 0x0A)
[DEVICE] E2E:KEEPALIVE count=1
✓ Keep-alive 1
[DEVICE] E2E:DONE PASS time_ms=45000

✓ Device test PASSED (45000ms)

📡 Verifying SmartLayerProxy telemetry...
  Found 5 telemetry events
  ✓ Firmware version in telemetry: e2e-a1b2c3d
  ✓ Firmware version matches device
  ✓ Diagnostics found in telemetry
    - Boot count: 2
    - WiFi attempts: 1
    - Connect time: 3245ms
  ✓ Telemetry verification successful

============================================================
E2E TEST SUMMARY
============================================================
Device Address:     0x1234...
Firmware Version:   e2e-a1b2c3d
WiFi Connected:     YES
TCP Login:          YES (0x09)
Diagnostics Sent:   YES
Keep-alive Count:   3
API Call Count:     0
Test Time:          45000ms

Telemetry Verification:
  Verified:         YES
  Firmware Match:   YES
  Diagnostics:      YES

OVERALL RESULT:     ✓ PASS
============================================================
```

## Telemetry Verification

When `TELEMETRY_API_KEY` is configured, the test runner verifies SmartLayerProxy recorded:

1. **Connect event** with firmware version matching device
   - Endpoint: `GET /api/telemetry/feed?afterId=0&limit=100`
   - Filters events for this device during the test run
   - Verifies `firmwareVersion === FW_VERSION`

2. **Diagnostics event** with boot count, WiFi attempts, connect time
   - Endpoint: `GET /api/telemetry/diagnostics/feed?afterId=0&limit=50`
   - Filters diagnostics for this device with `timestamp >= testStartTime`
   - Confirms opcode 0x0A data was received and recorded

**Authentication:**

Both telemetry endpoints require:

```
Authorization: Bearer <TELEMETRY_API_KEY>
```

**Connect Event Response:**

```json
{
  "rows": [
    {
      "device": "0x...",
      "eventType": "connect",
      "firmwareVersion": "e2e-a1b2c3d",
      "timestamp": 1700179200000
    }
  ]
}
```

**Diagnostics Event Response:**

```json
{
  "rows": [
    {
      "device": "0x...",
      "bootCount": 2,
      "wifiAttempts": 1,
      "connectTimeMs": 3245,
      "networkIndex": 0,
      "timestamp": 1700179200100
    }
  ]
}
```

**Note:** Telemetry verification is optional but recommended. The test will still pass if telemetry checks fail or are skipped due to missing API key.

## Troubleshooting

### KeyID not found

```
E2E:FAIL reason="KeyID not found - device must be pre-provisioned"
```

**Solution:** Flash Web3E-Setup-Firmware first to generate and store the private key.

### WifiStore invalid

```
E2E:FAIL reason="WifiStore invalid - device must be pre-provisioned"
```

**Possible causes:**
1. Device not provisioned with Web3E-Setup-Firmware
2. **EEPROM size mismatch** - KeyID truncated the WifiStore (see below)

**Solutions:**
- Use Web3E-Setup-Firmware to configure WiFi networks
- Verify EEPROM size: Check `E2E:EEPROM_SIZE=1024` in serial output
- If size is wrong, rebuild with correct `WEB3E_EEPROM_SIZE`

**CRITICAL Bug:** On arduino-esp32 3.x, if KeyID calls `EEPROM.begin(33)` before WifiStore, it truncates the NVS blob and destroys WifiStore data at offset 33+. This is fixed by using shared `WEB3E_EEPROM_SIZE=1024` in both components. If you see CRC errors after provisioning, this was likely the cause - re-provision the device.

### WiFi connection failed

```
E2E:FAIL reason="WiFi connection failed"
```

**Solutions:**
- Check WiFi credentials in WifiStore
- Verify networks are in range
- Check ESP32 WiFi antenna

### TCP connection timeout

```
E2E:FAIL reason="State timeout"
```

**Solutions:**
- Verify `PROXY_BASE_URL` is correct (default: https://scriptproxy.percolate.one, TCP 8082)
- Check firewall allows outbound TCP on port 8082
- Verify SmartLayerProxy is running and accessible

### Serial port errors

```
Serial port error: Error: No such file or directory
```

**Solutions:**
- Check COM port name: `COM3` (Windows), `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-*` (macOS)
- List ports: `pio device list`
- Check USB drivers installed
- Try a different USB cable/port

## Build Configuration

**platformio.ini:**

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    symlink://../../../  # Web3E library

build_flags =
    -DARDUINO_LOOP_STACK_SIZE=16384
    -DPROXY_HOST=\"scriptproxy.percolate.one\"
    -DPROXY_PORT=8082
    '-DFW_VERSION="e2e-${PIOENV}"'

extra_scripts = pre:git_version.py
```

The build script `git_version.py` injects the git short SHA as firmware version:

```python
git_sha = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'])
fw_version = f"e2e-{git_sha}"
```

## Exit Codes

The test runner exits with:
- `0` - Test passed
- `1` - Test failed or error

Use in CI/CD:

```bash
npm test COM3 || exit 1
```

## License

Same as Web3E parent project.
