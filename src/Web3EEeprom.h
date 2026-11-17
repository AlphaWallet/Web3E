#ifndef WEB3E_EEPROM_H
#define WEB3E_EEPROM_H

/**
 * Web3E EEPROM Layout and Size Constants
 * 
 * CRITICAL: All Web3E components (KeyID, WifiStore) MUST use the same
 * EEPROM size when calling EEPROM.begin(). On arduino-esp32 3.x, 
 * EEPROM.begin() with a smaller size than previously stored will
 * TRUNCATE the NVS blob, destroying data at higher offsets.
 * 
 * EEPROM Layout:
 * - Bytes 0-32:   KeyID (marker byte + 32-byte private key)
 * - Bytes 33+:    WifiStore (magic, version, networks, terminator, CRC32)
 * 
 * Size Calculation:
 * - KeyID: 33 bytes
 * - WifiStore max: ~795 bytes (8 networks × 98 bytes + 11 bytes overhead)
 * - Total: 33 + 795 = 828 bytes
 * - Rounded to: 1024 bytes (power of 2, leaves room for future expansion)
 */

// Default EEPROM size - can be overridden via build flag
#ifndef WEB3E_EEPROM_SIZE
#define WEB3E_EEPROM_SIZE 1024
#endif

// Validate minimum size
#if WEB3E_EEPROM_SIZE < 828
#error "WEB3E_EEPROM_SIZE must be at least 828 bytes to fit KeyID + WifiStore"
#endif

#endif // WEB3E_EEPROM_H
