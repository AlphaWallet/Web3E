# WiFi Portal Module

The Web3E WiFi Portal is an optional embedded captive portal that provides offline WiFi reconfiguration for ESP32 devices. This module is designed to be **completely optional** - applications that don't use it pay zero flash cost.

## Features

- **Triple Power-Cycle Reset**: Power-cycle the device 3 times within 10 seconds to open the portal
- **Lost-WiFi Watchdog**: Automatically opens portal if validated WiFi is lost for 5+ minutes
- **Revoke API**: Programmatically open portal on next reboot (no OTA to separate setup firmware needed)
- **Multi-Network Support**: Store up to 8 WiFi networks in EEPROM (bytes 33-827)
- **EEPROM Compatibility**: Binary-compatible with v1.47 layout (KeyID 0-32, WifiStore 33-827, RPC cache 828+)
- **NVS-Based Control**: Uses `web3e_boot` namespace for `stay_portal`, `boot_count`, `portal_reason`

## Usage

### 1. Enable the Module

Include the portal header in your sketch:

```cpp
#define WEB3E_WIFI_PORTAL
#include <Web3EWifiPortal.h>
```

### 2. Initialize in setup()

```cpp
Web3EWifiPortal wifiPortal;
WiFiConnection wifiConnection;

void setup() {
    // Initialize portal boot control
    if (!wifiPortal.begin()) {
        Serial.println("Portal init failed");
    }
    
    // Check if portal should open
    if (wifiPortal.shouldOpenPortal()) {
        // Get device address for display (optional)
        String addr = keyId->getAddress().c_str();
        
        // Run portal (blocks until user clicks Done)
        bool saved = wifiPortal.run(0);
        
        // Reboot to normal operation
        ESP.restart();
    }
    
    // Normal WiFi setup
    wifiConnection.setWifiPortal(&wifiPortal);
    wifiConnection.setupWifi(networks);
    
    // Mark WiFi as validated (arms watchdog)
    wifiConnection.markValidated();
}
```

### 3. Maintain WiFi in loop()

```cpp
void loop() {
    // Call maintain() periodically (e.g., every 1 second)
    wifiConnection.maintain();
    
    // Your application logic...
}
```

## Portal Triggers

The portal opens in three scenarios:

### Triple Power-Cycle (Offline Reset)

1. Power off the device
2. Power on
3. Wait < 10 seconds and repeat steps 1-2 two more times
4. Device enters portal mode and stays until user clicks Done

**Use case**: User lost WiFi password or moved device to new location without access to original network.

### Lost-WiFi Watchdog

After WiFi is validated (`markValidated()`):

- If disconnected, scans every 30-60 seconds for saved SSIDs
- If a saved SSID is visible → attempts reconnect, resets timer
- If scan returns 0 networks → timer doesn't advance (antenna/radio fault guard)
- If other SSIDs visible but no saved SSID for 5 continuous minutes → opens portal with `PORTAL_REASON_LOST_WIFI`
- Saved SSID visible but connect/internet failing → NEVER opens portal

**Use case**: User changed router SSID or moved device to new network, but device never sees the old SSID.

### Programmatic Revoke

```cpp
wifiPortal.requestPortal(PORTAL_REASON_REVOKE);
ESP.restart();
```

**Use case**: Admin revoked device's access; device reboots into portal for user to re-provision.

## Portal Behavior

- Opens softAP: `ESP32-Setup-XXXX` (last 4 hex digits of MAC)
- IP: `192.168.4.1` (open/no password by default)
- Captive portal redirects all HTTP requests to setup page
- Background WiFi scanning on AP channel (avoids softAP flake)
- For `PORTAL_REASON_LOST_WIFI`: auto-exits if saved SSID reappears and connects
- For `PORTAL_REASON_REVOKE` or `PORTAL_REASON_POWER_CYCLE`: stays until user clicks Done

## API Reference

### Web3EWifiPortal

```cpp
class Web3EWifiPortal {
public:
    bool begin();
    bool run(uint32_t timeoutMs = 0);
    bool isActive() const;
    bool requestPortal(PortalReason reason);
    PortalReason getPortalReason() const;
    bool shouldOpenPortal() const;
};
```

### WiFiConnection Extensions

```cpp
class WiFiConnection {
public:
    void maintain();                              // Call from loop() - reconnects + watchdog
    void markValidated();                         // Arms lost-WiFi watchdog
    void setLostWifiTimeout(uint32_t minutes);    // Default: 5 minutes
    void setWifiPortal(Web3EWifiPortal* portal);  // Link portal for watchdog
};
```

### PortalReason Enum

```cpp
enum PortalReason {
    PORTAL_REASON_NONE = 0,
    PORTAL_REASON_REVOKE = 1,
    PORTAL_REASON_POWER_CYCLE = 2,
    PORTAL_REASON_LOST_WIFI = 3
};
```

## Flash Cost

The portal module is designed to be **zero-cost when unused**:

- Applications that don't include `Web3EWifiPortal.h` pay **0 bytes** (linker removes unused code)
- Applications using the portal: **~45-50 KB** flash increase (DNSServer, WebServer, portal HTML/logic)
- Measured on `examples/Universal Node RPC`:
  - Without portal: 1,048,576 bytes
  - With portal: 1,094,144 bytes (+45,568 bytes)

## Customization

Override defaults via build flags:

```cpp
// AP SSID prefix (default: "ESP32-Setup")
#define AP_SSID_PREFIX "MyDevice-Setup"

// AP password (default: "" = open)
#define AP_PASSWORD "my_setup_password"

// App name shown in portal UI (default: "Web3E-Device")
#define APP_NAME "My IoT Device"
```

## Example

See `examples/WifiPortal/src/main.cpp` for a complete working example.

## Testing

### Triple Power-Cycle Test

1. Flash firmware
2. Power on (boot 1) - device fails WiFi, reboots
3. Power on (boot 2) - reboot
4. Power on (boot 3) - portal opens
5. Connect to `ESP32-Setup-XXXX` AP
6. Navigate to `192.168.4.1`
7. Add network, click Done
8. Device reboots and connects

### Lost-WiFi Watchdog Test

1. Flash firmware with valid WiFi credentials
2. Device connects, call `markValidated()`
3. Change router SSID or move device out of range
4. Wait 5+ minutes while device scans
5. Portal opens automatically
6. Add new network, click Done

### Revoke Test

1. Send serial command `r` (or call API)
2. Device sets portal flag and reboots
3. Portal opens immediately
4. Add network, click Done

## EEPROM Layout

The module maintains Web3E's existing layout:

```
Byte 0-32:   KeyID (marker + 32-byte private key)
Byte 33-827: WifiStore (magic, version, networks, terminator, CRC32)
Byte 828+:   RpcPairStore (timestamp, chainId, URLs)
```

**NEVER** touches KeyID or RPC cache regions. The portal writes only to WifiStore (33-827).

## NVS Keys

Namespace: `web3e_boot`

- `stay_portal` (uint8_t): 1 = stay in portal until Done
- `boot_count` (uint8_t): consecutive quick-reboot counter (clears after 10s uptime)
- `portal_reason` (uint8_t): reason portal was requested (revoke/power_cycle/lost_wifi)

## Board Support

Tested on:
- ESP32 (esp32dev)
- ESP32-S3
- ESP32-C3

Requires:
- `huge_app` or `min_spiffs` partition scheme for firmware size
- Arduino framework
- PlatformIO or Arduino IDE

## Troubleshooting

### Portal won't open on triple power-cycle

- Ensure power-off/on cycles happen within 10 seconds
- Check serial output for `boot_count` incrementing
- Verify NVS is not corrupted (erase flash and re-flash)

### Lost-WiFi watchdog not triggering

- Call `markValidated()` after WiFi connects
- Call `maintain()` from `loop()` at least every few seconds
- Ensure saved SSID is truly not visible (check serial scan results)

### Flash overflow

- Use `board_build.partitions = huge_app.csv` in `platformio.ini`
- Or switch to `min_spiffs` partition scheme
- Disable other large features if needed

## See Also

- [Web3E README](../README.md)
- [WifiStore Documentation](../src/WifiStore.h)
- [Example Code](../examples/WifiPortal/src/main.cpp)
