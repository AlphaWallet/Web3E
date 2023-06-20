//
// Created by James Brown on 2018/09/13.
//

#include <Arduino.h>
#include "Crypto.h"
#include "Web3.h"
#include "Util.h"
#include "Trezor/secp256k1.h"
#include "Trezor/ecdsa.h"
#include <vector>
#include <string.h>

#define SIGNATURE_LENGTH 64

using namespace std;

const char * PERSONAL_MESSAGE_PREFIX = "\u0019Ethereum Signed Message:\n";

Crypto::Crypto(Web3* _web3) 
{
    web3 = _web3;
    memset(privateKey, 0, ETHERS_PRIVATEKEY_LENGTH);
}

bool Crypto::Sign(BYTE* digest, BYTE* result) 
{
    const ecdsa_curve *curve = &secp256k1;
    uint8_t pby;
    int res = 0;
    bool allZero = true;
    for (int i = 0; i < ETHERS_PRIVATEKEY_LENGTH; i++) if (privateKey[i] != 0) allZero = false;
    if (allZero == true)
    {
        Serial.println("Private key not set, generate a private key (eg use Metamask) and call Contract::SetPrivateKey with it.");
    }
    else
    {
        res = ecdsa_sign_digest(curve, privateKey, digest, result, &pby, NULL);
        result[64] = pby;
    }

	return (res == 1);
}

void Crypto::SetPrivateKey(const char *key) 
{
    Util::ConvertHexToBytes(privateKey, key, ETHERS_PRIVATEKEY_LENGTH);
}

void Crypto::ECRecover(BYTE* signature, BYTE *public_key, BYTE *message_hash)
{
    uint8_t v = signature[64]; //recover 'v' from end of signature
	uint8_t pubkey[65];
    uint8_t signature64[64];
    if (v > 26) v -= 27; //correct value of 'v' if required
    memcpy(signature64, signature, 64);
	const ecdsa_curve *curve = &secp256k1;
	ecdsa_recover_pub_from_sig (curve, pubkey, signature, message_hash, v);
    memcpy(public_key, pubkey+1, 64);
}

bool Crypto::Verify(const uint8_t *public_key, const uint8_t *message_hash, const uint8_t *signature)
{
    const ecdsa_curve *curve = &secp256k1;
    BYTE publicKey4[65];
    memcpy(publicKey4+1, public_key, 64);
    publicKey4[0] = 4;
    int success = ecdsa_verify_digest(curve, publicKey4, signature, message_hash);
    return (success == 0);
}

void Crypto::PrivateKeyToPublic(const uint8_t *privateKey, uint8_t *publicKey)
{
    uint8_t buffer[65];
    const ecdsa_curve *curve = &secp256k1;
    ecdsa_get_public_key65(curve, privateKey, buffer);
    memcpy(publicKey, buffer+1, 64);
}

void Crypto::PublicKeyToAddress(const uint8_t *publicKey, uint8_t *address)
{
    uint8_t hashed[32];
    Keccak256(publicKey, 64, hashed);
    memcpy(address, &hashed[12], 20);
}

void Crypto::Keccak256(const uint8_t *data, uint16_t length, uint8_t *result)
{
    keccak_256(data, length, result);
}

string Crypto::Keccak256(vector<uint8_t> *bytes)
{
    uint8_t result[ETHERS_KECCAK256_LENGTH];
    keccak_256(bytes->data(), bytes->size(), result);
    //now convert result to hex string

    vector<uint8_t> resultVector;
    for (int i = 0; i < ETHERS_KECCAK256_LENGTH; i++) resultVector.push_back(result[i]);

    return Util::VectorToString(&resultVector);
}

string Crypto::ECRecoverFromHexMessage(string *signature, string *message)
{
    uint8_t challengeHash[ETHERS_KECCAK256_LENGTH];
    //convert hex to bytes
    vector<uint8_t> messageBytes = Util::ConvertHexToVector((uint8_t*)(message->c_str()));
    //hash the full challenge    
    Crypto::Keccak256((uint8_t*)messageBytes.data(), messageBytes.size(), challengeHash);
    return ECRecoverFromHash(signature, challengeHash);
}

string Crypto::ECRecoverFromHash(string *signature, BYTE *digest)
{
    //convert address to BYTE
    BYTE signatureBytes[ETHERS_SIGNATURE_LENGTH];
    BYTE publicKeyBytes[ETHERS_PUBLICKEY_LENGTH];

    Util::ConvertHexToBytes(signatureBytes, signature->c_str(), ETHERS_SIGNATURE_LENGTH);

    // Perform ECRecover to get the public key - this extracts the public key of the private key
    // that was used to sign the message - that is, the private key held by the wallet/metamask
    Crypto::ECRecover(signatureBytes, publicKeyBytes, digest); 

    BYTE recoverAddr[ETHERS_ADDRESS_LENGTH];
    // Now reduce the recovered public key to Ethereum address
    Crypto::PublicKeyToAddress(publicKeyBytes, recoverAddr);
    string recoveredAddress = Util::ConvertBytesToHex(recoverAddr, ETHERS_ADDRESS_LENGTH);
    return recoveredAddress;
}

string Crypto::ECRecoverFromPersonalMessage(string *signature, string *message)
{
    uint8_t challengeHash[ETHERS_KECCAK256_LENGTH];
    string challengeStr = PERSONAL_MESSAGE_PREFIX;
    challengeStr += Util::toString((int)message->length());
    challengeStr += *message;

    Crypto::Keccak256((uint8_t*)challengeStr.c_str(), challengeStr.length(), challengeHash);
    return ECRecoverFromHash(signature, challengeHash);
}