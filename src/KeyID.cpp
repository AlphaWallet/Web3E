#include <KeyID.h>
#include "Util.h"
#include "Web3Log.h"

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

  // CRITICAL: Use WEB3E_EEPROM_SIZE to avoid truncating WifiStore at offset 33+
  // On arduino-esp32 3.x, EEPROM.begin() with a smaller size truncates the NVS blob
  if (!EEPROM.begin(WEB3E_EEPROM_SIZE))
  {
    WEB3E_PRINTLN("failed to initialise EEPROM");
    delay(1000000);
    return;
  }
  
  WEB3E_PRINT("KeyID: EEPROM initialized with size ");
  WEB3E_PRINTLN(WEB3E_EEPROM_SIZE);

  //fitst see if there's a stored key
  if (static_cast<uint8_t>(EEPROM.read(0)) == 64)
  {
    WEB3E_PRINTLN();
    WEB3E_PRINTLN("Recovering key from EEPROM");
    for (int i = 0; i < ETHERS_PRIVATEKEY_LENGTH; i++)
    {
      privateKeyBytes[i] = static_cast<uint8_t>(EEPROM.read(i + 1));
    }

    privateKey = Util::ConvertBytesToHex(privateKeyBytes, ETHERS_PRIVATEKEY_LENGTH);

    if (privateKey[1] == 'x') privateKey = privateKey.substr(2);

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
  WEB3E_PRINT("Got Hash: ");
  WEB3E_PRINTLN(Util::ConvertBytesToHex(hash, ETHERS_KECCAK256_LENGTH).c_str());
  crypto->Sign(hash, signature);
  WEB3E_PRINT("Got Sig: ");
  WEB3E_PRINTLN(Util::ConvertBytesToHex(signature, ETHERS_SIGNATURE_LENGTH).c_str());
}

void KeyID::initPrivateKey(const std::string& privateKey, Web3* web3)
{
  crypto = new Crypto(web3);
  crypto->SetPrivateKey(privateKey.c_str());
  recoveredKey = true;
  WEB3E_PRINT("Device Address: ");
  WEB3E_PRINTLN(getAddress().c_str());
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