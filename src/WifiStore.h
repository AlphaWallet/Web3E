#ifndef WIFISTORE_H
#define WIFISTORE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

// EEPROM Layout Constants
// KeyID (Web3E) uses bytes 0-32: [0]=marker(64), [1-32]=32-byte private key
#define KEYID_EEPROM_SIZE 33
#define WIFI_EEPROM_OFFSET 33

// Wi-Fi EEPROM structure (starts at offset 33):
// [33-36]: Magic "WIFI" (4 bytes)
// [37]:    Version byte (currently 1)
// [38+]:   Records: [ssidLen:1][ssid bytes][pwLen:1][pw bytes]
// Terminator: 0x00 0x00 (empty SSID + empty password)
// Last 4 bytes: CRC32 over magic..terminator (little-endian)

#define WIFI_MAGIC_SIZE 4
#define WIFI_VERSION_SIZE 1
#define WIFI_HEADER_SIZE (WIFI_MAGIC_SIZE + WIFI_VERSION_SIZE)
#define WIFI_CRC_SIZE 4
#define WIFI_TERMINATOR_SIZE 2

#ifndef MAX_WIFI_NETWORKS
#define MAX_WIFI_NETWORKS 8
#endif

#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

// Calculate required EEPROM size
#define WIFI_MAX_STORAGE_SIZE (WIFI_HEADER_SIZE + (MAX_WIFI_NETWORKS * (1 + MAX_SSID_LENGTH + 1 + MAX_PASSWORD_LENGTH)) + WIFI_TERMINATOR_SIZE + WIFI_CRC_SIZE)
#define TOTAL_EEPROM_SIZE (WIFI_EEPROM_OFFSET + WIFI_MAX_STORAGE_SIZE)

struct WifiCredential {
    char ssid[MAX_SSID_LENGTH + 1];
    char password[MAX_PASSWORD_LENGTH + 1];
    uint8_t ssidLen;
    uint8_t pwLen;
};

class WifiStore {
public:
    WifiStore();
    
    // Initialize and read stored networks
    bool begin();
    
    // Read all stored networks
    int readNetworks(WifiCredential* credentials, int maxCount);
    
    // Validate stored data (checks magic, version, and CRC)
    bool isValid();
    
    // Get count of stored networks
    int getNetworkCount();

private:
    bool isInitialized;
    
    // CRC32 calculation (matches Web3E-Setup-Firmware exactly)
    uint32_t calculateCRC32(const uint8_t* data, size_t length);
    
    // Internal helpers
    int readStoredNetworks(WifiCredential* credentials, int maxCount, bool validate);
};

#endif // WIFISTORE_H
