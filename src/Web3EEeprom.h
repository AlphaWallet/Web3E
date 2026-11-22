#ifndef WEB3E_EEPROM_H
#define WEB3E_EEPROM_H

/**
 * Web3E EEPROM Layout and Size Constants
 *
 * CRITICAL: All Web3E components (KeyID, WifiStore, RpcPairStore) MUST use the
 * same EEPROM size when calling EEPROM.begin(). On arduino-esp32 3.x,
 * EEPROM.begin() with a smaller size than previously stored will
 * TRUNCATE the NVS blob, destroying data at higher offsets.
 *
 * EEPROM Layout:
 * - Bytes 0-32:     KeyID (marker byte + 32-byte private key)
 * - Bytes 33-827:   WifiStore reserved region (magic, version, networks,
 *                   terminator, CRC32). Max WIFI_MAX_STORAGE_SIZE = 795 bytes.
 * - Bytes 828+:     RpcPairStore (timestamp, chainId, length-prefixed HTTPS URLs)
 *
 * Size Calculation:
 * - KeyID: 33 bytes
 * - WifiStore max: 795 bytes → ends at offset 828
 * - RpcPairStore max: 4 (ts) + 4 (chainId) + 2×(1+255) + 1 (terminator) = 521
 * - Total minimum: 828 + 521 = 1349 bytes
 * - Default: 2048 bytes (power of 2, room for expansion)
 *
 * Never touch bytes 0-32 (KeyID) or the WifiStore region from RpcPairStore.
 * Never full-erase flash from these helpers.
 */

// Default EEPROM size - can be overridden via build flag
#ifndef WEB3E_EEPROM_SIZE
#define WEB3E_EEPROM_SIZE 2048
#endif

// KeyID + WifiStore reserved end (see WifiStore.h WIFI_MAX_STORAGE_SIZE)
#ifndef WEB3E_WIFI_STORE_END
#define WEB3E_WIFI_STORE_END 828
#endif

// RpcPairStore maximum payload (timestamp + chainId + 2 URLs + terminator)
#ifndef WEB3E_RPC_PAIR_MAX_SIZE
#define WEB3E_RPC_PAIR_MAX_SIZE 521
#endif

#if WEB3E_EEPROM_SIZE < (WEB3E_WIFI_STORE_END + WEB3E_RPC_PAIR_MAX_SIZE)
#error "WEB3E_EEPROM_SIZE must be at least 1349 bytes to fit KeyID + WifiStore + RpcPairStore"
#endif

#endif // WEB3E_EEPROM_H
