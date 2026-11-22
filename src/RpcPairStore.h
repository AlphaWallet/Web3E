#ifndef RPC_PAIR_STORE_H
#define RPC_PAIR_STORE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>
#include "Web3EEeprom.h"
#include "WifiStore.h"

/**
 * EEPROM cache for a signed best-RPC URL pair from universal-node.
 *
 * Placed immediately after the WifiStore reserved region:
 *   RPC_PAIR_EEPROM_OFFSET = WIFI_EEPROM_OFFSET + WIFI_MAX_STORAGE_SIZE = 828
 *
 * Layout (little-endian integers):
 *   [offset+0 .. +3]  uint32 timestamp  — unix secs when the pair was issued/fetched
 *   [offset+4 .. +7]  uint32 chainId    — forces refresh on chain change (spec addition)
 *   [offset+8 ..]     length-prefixed HTTPS URL records:
 *                       1 byte length (1..255) + URL bytes
 *                       repeated for 1 or 2 URLs
 *                       terminated by a 0 length byte
 *
 * Blank / 0xFF / out-of-bounds / corrupt data is treated as empty.
 * Never writes KeyID (0-32) or WifiStore (33..827). Commits only when bytes change.
 */

#define RPC_PAIR_EEPROM_OFFSET (WIFI_EEPROM_OFFSET + WIFI_MAX_STORAGE_SIZE) /* 828 */
#define RPC_PAIR_MAX_URLS 2
#define RPC_PAIR_MAX_URL_LENGTH 255
#define RPC_PAIR_TIMESTAMP_SIZE 4
#define RPC_PAIR_CHAIN_ID_SIZE 4
#define RPC_PAIR_HEADER_SIZE (RPC_PAIR_TIMESTAMP_SIZE + RPC_PAIR_CHAIN_ID_SIZE)
#define RPC_PAIR_STORAGE_SIZE WEB3E_RPC_PAIR_MAX_SIZE

struct RpcPairData {
    uint32_t timestamp;
    uint32_t chainId;
    char urls[RPC_PAIR_MAX_URLS][RPC_PAIR_MAX_URL_LENGTH + 1];
    uint8_t urlLens[RPC_PAIR_MAX_URLS];
    uint8_t count;
};

class RpcPairStore {
public:
    RpcPairStore();

    bool begin();

    /** Load cache into out. Returns false if empty/corrupt. */
    bool load(RpcPairData& out);

    /**
     * Persist pair. Validates https URLs (len 1..255). Commits EEPROM only
     * when the written region differs from what is stored.
     */
    bool save(uint32_t timestamp, uint32_t chainId, const char* const* urls, int count);

    /** True when region is blank, 0xFF-filled, or fails validation. */
    bool isEmpty();

private:
    bool isInitialized;
    bool readU32(int offset, uint32_t& out) const;
    void writeU32(int offset, uint32_t value);
    bool regionLooksBlank(int offset, int length) const;
    bool validateUrl(const char* url, uint8_t len) const;
};

#endif // RPC_PAIR_STORE_H
