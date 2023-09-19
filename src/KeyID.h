#ifndef KEY_ID_H
#define KEY_ID_H
#include "EEPROM.h"
#include <Web3.h>
#include <Trezor/rand.h>
#include <string>
#include <Crypto.h>

#define EEPROM_SIZE ETHERS_PRIVATEKEY_LENGTH

class KeyID 
{
public:
    KeyID(Web3* web3);
    KeyID(Web3* web3, const std::string& privateKey);
    void generatePrivateKey(Web3* web3);
    void getSignature(uint8_t* signature, BYTE *msgBytes, int length);
    const std::string getAddress();
    bool hasRecoveredKey() { return recoveredKey; };
	
private:
    void initPrivateKey(const std::string& privateKey, Web3* web3);

    BYTE *privateKeyBytes;
    bool recoveredKey;
    Crypto *crypto;
};

#endif