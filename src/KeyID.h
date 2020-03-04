#ifndef KEY_ID_H
#define KEY_ID_H
#include "EEPROM.h"
#include <web3.h>
#include <trezor/rand.h>
#include <string>
#include <Crypto.h>

#define EEPROM_SIZE ETHERS_PRIVATEKEY_LENGTH

class KeyID 
{
public:
    KeyID(Web3* web3);
    void generatePrivateKey(Web3* web3);
    void getSignature(uint8_t* signature, BYTE *msgBytes);
    const std::string getAddress();
    bool hasRecoveredKey() { return recoveredKey; };
	
private:
    void initPrivateKey(const std::string& privateKey, Web3* web3);

    BYTE *privateKeyBytes;
    bool recoveredKey;
    Crypto *crypto;
};

#endif