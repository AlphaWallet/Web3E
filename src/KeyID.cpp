#include <KeyID.h>
#include "Util.h"

// If we already have a private key
KeyID::KeyID(Web3* web3, const std::string& privateKey)
{
  privateKeyBytes = new BYTE[ETHERS_PRIVATEKEY_LENGTH];
  Util::ConvertHexToBytes(privateKeyBytes, privateKey.c_str(), ETHERS_PRIVATEKEY_LENGTH);
  initPrivateKey(privateKey, web3);
}

KeyID::KeyID(Web3* web3)
{
  recoveredKey = false;
  //private key is two 64 bytes, can do this by generating random bytes until we have 64 hex digits
  std::string privateKey = "";
  privateKeyBytes = new BYTE[ETHERS_PRIVATEKEY_LENGTH];

  if (!EEPROM.begin(EEPROM_SIZE + 1))
  {
    Serial.println("failed to initialise EEPROM");
    delay(1000000);
    return;
  }

  //fitst see if there's a stored key
  if (byte(EEPROM.read(0)) == 64)
  {
    Serial.println();
    Serial.println("Recovering key from EEPROM");
    for (int i = 0; i < ETHERS_PRIVATEKEY_LENGTH; i++)
    {
      privateKeyBytes[i] = byte(EEPROM.read(i + 1));
    }

    privateKey = Util::ConvertBytesToHex(privateKeyBytes, ETHERS_PRIVATEKEY_LENGTH);

    if (privateKey[1] == 'x') privateKey.substr(2);

    initPrivateKey(privateKey, web3);
  }
  else
  {
    generatePrivateKey(web3);
  }
}

void KeyID::generatePrivateKey(Web3* web3)
{
  random_buffer(privateKeyBytes, ETHERS_PRIVATEKEY_LENGTH);

  std::string privateKey = Util::ConvertBytesToHex(privateKeyBytes, ETHERS_PRIVATEKEY_LENGTH);

  EEPROM.write(0, 64);
  for (int i = 0; i < ETHERS_PRIVATEKEY_LENGTH; i++)
  {
    EEPROM.write(i + 1, privateKeyBytes[i]);
  }

  EEPROM.commit();

  initPrivateKey(privateKey, web3);
}

void KeyID::getSignature(uint8_t* signature, BYTE* msgBytes, int length)
{
  if (recoveredKey == false) return;
  //need to convert uint32_t to byte
  uint8_t hash[ETHERS_KECCAK256_LENGTH];

  Crypto::Keccak256((uint8_t*)msgBytes, length, hash);
  Serial.print("Got Hash: ");
  Serial.println(Util::ConvertBytesToHex(hash, ETHERS_KECCAK256_LENGTH).c_str());
  crypto->Sign(hash, signature);
  Serial.print("Got Sig: ");
  Serial.println(Util::ConvertBytesToHex(signature, ETHERS_SIGNATURE_LENGTH).c_str());
}

void KeyID::initPrivateKey(const std::string& privateKey, Web3* web3)
{
  crypto = new Crypto(web3);
  crypto->SetPrivateKey(privateKey.c_str());
  recoveredKey = true;
  Serial.print("Device Address: ");
  Serial.println(getAddress().c_str());
}

const std::string KeyID::getAddress()
{
  BYTE ethAddressBytes[ETHERS_PUBLICKEY_LENGTH];
  BYTE publicKey[ETHERS_PUBLICKEY_LENGTH];
  Crypto::PrivateKeyToPublic(privateKeyBytes, publicKey);
  Crypto::PublicKeyToAddress(publicKey, ethAddressBytes);
  std::string address = Util::ConvertBytesToHex(ethAddressBytes, ETHERS_ADDRESS_LENGTH);
  return address;
}