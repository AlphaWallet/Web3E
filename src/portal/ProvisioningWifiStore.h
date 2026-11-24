#ifndef PROVISIONINGWIFISTORE_H
#define PROVISIONINGWIFISTORE_H

#include <stdint.h>
#include <string.h>
#include "../WifiStore.h"

class ProvisioningWifiStore {
public:
    ProvisioningWifiStore();

    bool begin();
    int readNetworks(WifiCredential* credentials, int maxCount);
    bool writeNetworks(const WifiCredential* credentials, int count);
    bool appendNetwork(const char* ssid, const char* password);
    bool removeNetwork(int index);
    bool clearNetworks();
    int getNetworkCount();
    bool isValid();

private:
    bool isInitialized;

    uint32_t calculateCRC32(const uint8_t* data, size_t length);
    uint8_t eepromRead(int address);
    void eepromWrite(int address, uint8_t value);
    bool eepromCommit();

    bool writeHeader();
    bool writeTerminator(int offset);
    bool writeCRC(int dataEndOffset);
    int readStoredNetworks(WifiCredential* credentials, int maxCount, bool validate);
};

#endif
