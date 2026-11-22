#include "RpcPairStore.h"
#include "Web3Log.h"
#include <string.h>

RpcPairStore::RpcPairStore() : isInitialized(false) {}

bool RpcPairStore::begin() {
    if (!EEPROM.begin(WEB3E_EEPROM_SIZE)) {
        WEB3E_PRINTLN("RpcPairStore: Failed to initialize EEPROM");
        return false;
    }
    isInitialized = true;
    WEB3E_PRINT("RpcPairStore: Initialized at offset ");
    WEB3E_PRINT(RPC_PAIR_EEPROM_OFFSET);
    WEB3E_PRINT(" EEPROM size ");
    WEB3E_PRINTLN(WEB3E_EEPROM_SIZE);
    return true;
}

bool RpcPairStore::readU32(int offset, uint32_t& out) const {
    if (offset < 0 || offset + 4 > WEB3E_EEPROM_SIZE) return false;
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        v |= (static_cast<uint32_t>(EEPROM.read(offset + i)) << (8 * i));
    }
    out = v;
    return true;
}

void RpcPairStore::writeU32(int offset, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        EEPROM.write(offset + i, static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }
}

bool RpcPairStore::regionLooksBlank(int offset, int length) const {
    if (offset < 0 || length <= 0 || offset + length > WEB3E_EEPROM_SIZE) return true;
    bool allZero = true;
    bool allFF = true;
    for (int i = 0; i < length; i++) {
        uint8_t b = EEPROM.read(offset + i);
        if (b != 0x00) allZero = false;
        if (b != 0xFF) allFF = false;
        if (!allZero && !allFF) return false;
    }
    return allZero || allFF;
}

bool RpcPairStore::validateUrl(const char* url, uint8_t len) const {
    // len is uint8_t so 1..255 is the full valid non-empty range (max URL length).
    if (!url || len == 0) return false;
    // Require https:// (case-sensitive per fetch reject rules)
    static const char kPrefix[] = "https://";
    const size_t prefixLen = sizeof(kPrefix) - 1;
    if (len <= prefixLen) return false;
    if (strncmp(url, kPrefix, prefixLen) != 0) return false;
    // Host must have at least one character after scheme
    if (url[prefixLen] == '\0' || url[prefixLen] == '/') return false;
    for (uint8_t i = 0; i < len; i++) {
        unsigned char c = static_cast<unsigned char>(url[i]);
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

bool RpcPairStore::isEmpty() {
    RpcPairData tmp;
    return !load(tmp);
}

bool RpcPairStore::load(RpcPairData& out) {
    memset(&out, 0, sizeof(out));
    if (!isInitialized) return false;

    const int base = RPC_PAIR_EEPROM_OFFSET;
    if (base + RPC_PAIR_HEADER_SIZE + 1 > WEB3E_EEPROM_SIZE) return false;

    // Entire header blank/0xFF → empty
    if (regionLooksBlank(base, RPC_PAIR_HEADER_SIZE)) return false;

    uint32_t timestamp = 0;
    uint32_t chainId = 0;
    if (!readU32(base, timestamp) || !readU32(base + 4, chainId)) return false;

    // timestamp 0 or 0xFFFFFFFF treated as empty
    if (timestamp == 0 || timestamp == 0xFFFFFFFFu) return false;

    int offset = base + RPC_PAIR_HEADER_SIZE;
    uint8_t count = 0;

    while (count < RPC_PAIR_MAX_URLS) {
        if (offset >= WEB3E_EEPROM_SIZE) return false;
        uint8_t len = EEPROM.read(offset++);
        if (len == 0) {
            // Terminator — success only if we have at least one URL
            break;
        }
        // len is uint8_t (1..255); RPC_PAIR_MAX_URL_LENGTH == 255
        if (offset + len > WEB3E_EEPROM_SIZE) return false;

        for (uint8_t i = 0; i < len; i++) {
            out.urls[count][i] = static_cast<char>(EEPROM.read(offset++));
        }
        out.urls[count][len] = '\0';
        out.urlLens[count] = len;

        if (!validateUrl(out.urls[count], len)) return false;
        count++;
    }

    if (count == 0) return false;

    // Must see terminator 0 after the last URL (already consumed in loop break),
    // or if we hit max URLs, next byte should be 0.
    if (count == RPC_PAIR_MAX_URLS) {
        if (offset >= WEB3E_EEPROM_SIZE) return false;
        uint8_t term = EEPROM.read(offset);
        if (term != 0) return false;
    }

    out.timestamp = timestamp;
    out.chainId = chainId;
    out.count = count;
    return true;
}

bool RpcPairStore::save(uint32_t timestamp, uint32_t chainId,
                        const char* const* urls, int count) {
    if (!isInitialized) return false;
    if (timestamp == 0 || timestamp == 0xFFFFFFFFu) return false;
    if (!urls || count < 1 || count > RPC_PAIR_MAX_URLS) return false;

    uint8_t lens[RPC_PAIR_MAX_URLS];
    for (int i = 0; i < count; i++) {
        if (!urls[i]) return false;
        size_t n = strlen(urls[i]);
        if (n == 0 || n > RPC_PAIR_MAX_URL_LENGTH) return false;
        lens[i] = static_cast<uint8_t>(n);
        if (!validateUrl(urls[i], lens[i])) return false;
    }

    const int base = RPC_PAIR_EEPROM_OFFSET;
    int needed = RPC_PAIR_HEADER_SIZE + 1; // header + terminator
    for (int i = 0; i < count; i++) needed += 1 + lens[i];
    if (base + needed > WEB3E_EEPROM_SIZE) return false;
    if (needed > RPC_PAIR_STORAGE_SIZE) return false;

    // Build desired image and compare to EEPROM before writing
    uint8_t image[RPC_PAIR_STORAGE_SIZE];
    memset(image, 0, sizeof(image));
    image[0] = static_cast<uint8_t>(timestamp & 0xFF);
    image[1] = static_cast<uint8_t>((timestamp >> 8) & 0xFF);
    image[2] = static_cast<uint8_t>((timestamp >> 16) & 0xFF);
    image[3] = static_cast<uint8_t>((timestamp >> 24) & 0xFF);
    image[4] = static_cast<uint8_t>(chainId & 0xFF);
    image[5] = static_cast<uint8_t>((chainId >> 8) & 0xFF);
    image[6] = static_cast<uint8_t>((chainId >> 16) & 0xFF);
    image[7] = static_cast<uint8_t>((chainId >> 24) & 0xFF);

    int pos = RPC_PAIR_HEADER_SIZE;
    for (int i = 0; i < count; i++) {
        image[pos++] = lens[i];
        memcpy(image + pos, urls[i], lens[i]);
        pos += lens[i];
    }
    image[pos++] = 0; // terminator

    bool changed = false;
    for (int i = 0; i < pos; i++) {
        if (EEPROM.read(base + i) != image[i]) {
            changed = true;
            break;
        }
    }
    if (!changed) {
        WEB3E_PRINTLN("RpcPairStore: unchanged, skip commit");
        return true;
    }

    for (int i = 0; i < pos; i++) {
        EEPROM.write(base + i, image[i]);
    }
    if (!EEPROM.commit()) {
        WEB3E_PRINTLN("RpcPairStore: EEPROM commit failed");
        return false;
    }

    WEB3E_PRINT("RpcPairStore: saved ");
    WEB3E_PRINT(count);
    WEB3E_PRINT(" RPC(s) chainId=");
    WEB3E_PRINT(chainId);
    WEB3E_PRINT(" ts=");
    WEB3E_PRINTLN(timestamp);
    return true;
}
