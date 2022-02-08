/*
 *  August lock control for ESP32
 *  by James Brown j@smarttokenlabs.com
 *
 *  Based on a C# implementation by Marcus Lum https://github.com/Marcus-L/xamarin-august-ble
 */

#include "august.h"

AugustLock::AugustLock(const char *deviceAddress, const char *handshakeKey, const uint8_t offlineKeyOffset)
{
    this->deviceAddress = deviceAddress;
    this->handshakeKey = handshakeKey;
    this->offlineKeyOffset = offlineKeyOffset;

    handshakeKeyBytes = new uint8_t[16];
    deviceCallback = new AdvertisedDeviceCallbacks(this);
}

void AugustLock::init()
{
    AUGUST_LOG("Init Bluetooth");
    NimBLEDevice::init("");
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_SC);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
}

void AugustLock::connect(ConnectCallback callback, notify_callback notifyCB, notify_callback secureLockCallback, DisConnectCallback dsCallback)
{
    // kick off scan
    AUGUST_LOG("Starting AugustLock BLE Library");
    resetCrypto();

    pNotifyCB = notifyCB;
    pSecureLockCallback = secureLockCallback;
    connectCallback = callback;
    dsCb = dsCallback;

    scanForService();
}

void AugustLock::scanForService()
{
    if (pClient != nullptr)
    {
        NimBLEDevice::deleteClient(pClient);
        pClient = NULL;
        advDevice = NULL;
    }

    advDevice = NULL;

    doConnect = false;
    doneConnect = false;

    AUGUST_LOG("Start Bluetooth scan");
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(deviceCallback);
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    pScan->start(0, scanEndedCB);
}

void AugustLock::lockAction(LockAction action)
{
    resetCrypto();

    doneConnect = false;
    if (advDevice != NULL)
    {
        doConnect = true;
    }

    actionAfterSync = action;
}

void AugustLock::closeConnection()
{
    if (pWriteHandshake->canWrite())
    {
        uint8_t *cmd = BuildCommand(0x05);
        memcpy(&cmd[4], &handshakeSessionKey[8], 8);
        SecWriteChecksum(cmd);

        AUGUST_LOG("DISCONNECT: %s", ConvertBytesToHex(cmd, 18).c_str());

        uint8_t secMessage[18];

        // encrypt
        encryptMessage(cmd, secMessage);

        delay(100);

        if (pWriteHandshake->writeValue(secMessage, 18, false))
        {
            AUGUST_LOG("Wrote new value to: %s", pWriteHandshake->getUUID().toString().c_str());
        }
        else
        {
            pClient->disconnect();
        }

        doConnect = false;
        pClient = NULL;
    }
}

void AugustLock::lockCommand(LockAction action)
{
    if (pWriteSecure->canWrite())
    {
        uint8_t *cmd = lockCmd(getLockCode(action));
        WriteChecksum(cmd);

        AUGUST_LOG("%s: %s", getLockActionStr(action), ConvertBytesToHex(cmd, 18).c_str());

        uint8_t lockMessage[18];

        // encrypt
        cbcEncryptMessage(cmd, lockMessage);

        delay(50);

        AUGUST_LOG("%s (encrypt): %s", getLockActionStr(action), ConvertBytesToHex(lockMessage, 18).c_str());

        AUGUST_LOG("Write for %s", getLockActionStr(action));
        if (pWriteSecure->writeValue(lockMessage, 18, false))
        {
            AUGUST_LOG("Wrote new value to: %s", pWriteSecure->getUUID().toString().c_str());
        }
        else
        {
            pClient->disconnect();
        }
    }
}

void AugustLock::getStatus()
{
    if (pWriteSecure->canWrite())
    {
        uint8_t *status = statusCmd();
        WriteChecksum(status);

        AUGUST_LOG("status: %s", ConvertBytesToHex(status, 18).c_str());

        uint8_t lockMessage[18];

        // encrypt
        cbcEncryptMessage(status, lockMessage);

        AUGUST_LOG("status(Encrypt): %s", ConvertBytesToHex(lockMessage, 18).c_str());

        if (pWriteSecure->writeValue(lockMessage, 18, false))
        {
            AUGUST_LOG("Wrote new value to: %s", pWriteSecure->getUUID().toString().c_str());
        }
        else
        {
            pClient->disconnect();
        }
    }
}

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results)
{
    AUGUST_LOG("Scan Ended");
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool AugustLock::connectToServer()
{
    /** No client to reuse? Create a new one. */
    if (!pClient || !pClient->isConnected())
    {
        pClient = NimBLEDevice::createClient();

        if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
        {
            AUGUST_LOG("Max clients reached - no more connections available");
            return false;
        }

        AUGUST_LOG("New client created");
    }
    else if (pClient->isConnected())
    {
        AUGUST_LOG("Already connected");
        return true;
    }

    pClient->setClientCallbacks(&clientCB, false);
    pClient->setConnectionParams(10, 12, 1, 300);
    pClient->setConnectTimeout(127);

    if (!pClient->connect(advDevice))
    {
        /** Created a client but failed to connect, don't need to keep it as it has no data */
        NimBLEDevice::deleteClient(pClient);
        pClient = NULL;
        advDevice = NULL;
        AUGUST_LOG("Failed to connect, deleted client");
        //need to re-do the scan
        scanForService();
        return false;
    }
    else
    {
        AUGUST_LOG("Connected to client: %s", pClient->getPeerAddress().toString().c_str());
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(advDevice))
        {
            AUGUST_LOG("Failed to connect");
            return false;
        }
    }

    NimBLERemoteService *pSvc = pClient->getService(serviceUUID);

    if (!pSvc)
        pSvc = pClient->getService(serviceUUID2); //check on abbreviated UUID

    if (pSvc)
    {
        NimBLERemoteCharacteristic *pReadLockCmd = pSvc->getCharacteristic(readSecure);
        NimBLERemoteCharacteristic *pReadSecureChannel = pSvc->getCharacteristic(readHandshake);
        pWriteSecure = pSvc->getCharacteristic(writeSecure);
        pWriteHandshake = pSvc->getCharacteristic(writeHandshake);

        if (pReadLockCmd)
        {
            if (pReadLockCmd->canIndicate())
            {
                /** Send false as first argument to subscribe to indications instead of notifications */
                if (!pReadLockCmd->subscribe(false, pSecureLockCallback))
                {
                    AUGUST_LOG("Lock command channel indicate fail");
                    /** Disconnect if subscribe failed */
                    pClient->disconnect();
                    pClient = NULL;
                }
                else
                {
                    AUGUST_LOG("Lock command channel notification listener success");
                }
            }
        }

        if (pReadSecureChannel)
        { /** make sure it's not null */
            AUGUST_LOG("Got Secure Channel");
            if (pReadSecureChannel->canIndicate())
            {
                AUGUST_LOG("Can Indicate");
                if (!pReadSecureChannel->subscribe(false, pNotifyCB))
                {
                    AUGUST_LOG("Handshake channel setup fail");
                    /** Disconnect if subscribe failed */
                    pClient->disconnect();
                    pClient = NULL;
                    return false;
                }
                else
                {
                    AUGUST_LOG("Handshake channel setup complete");
                }
            }
        }

        // initiate handshake with Lock, via secure channel
        if (pWriteHandshake)
        {
            uint8_t handshakeBytes[18];

            AUGUST_LOG("Got Write Channel");

            getHandshakeBytes(handshakeBytes);

            if (pWriteHandshake->canWriteNoResponse())
            {
                AUGUST_LOG("Write no response");
                if (pWriteHandshake->writeValue(handshakeBytes, 18, false))
                {
                    AUGUST_LOG("Wrote new value to: %s", pWriteHandshake->getUUID().toString().c_str());
                }
                else
                {
                    pClient->disconnect();
                    return false;
                }
            }
        }
    }
    else
    {
        AUGUST_LOG("Couldn't connect to August: %s", serviceUUID.toString().c_str());
    }

    AUGUST_LOG("Setup complete");
    return true;
}

void AugustLock::checkStatus()
{
    // TODO: check lock status periodically, to detect if open/closed
    if (doConnect)
    {
        if (!doneConnect && connectToServer())
        {
            AUGUST_LOG("Notification listener setup, awaiting instruction");
            doneConnect = true;
        }
    }
}

//
// Build command opcodes
//
uint8_t *AugustLock::BuildCommand(byte opcode)
{
    memset(commandBuffer, 0, 18);
    commandBuffer[0] = opcode;
    commandBuffer[16] = 0x0f; // unknown
    commandBuffer[17] = offlineKeyOffset;
    return commandBuffer;
}

uint8_t *AugustLock::lockCmd(uint8_t opcode)
{
    memset(commandBuffer, 0, 18);
    commandBuffer[0] = 0xee; // magic
    commandBuffer[1] = opcode;
    commandBuffer[16] = 0x02; // unknown
    return commandBuffer;
}

uint8_t *AugustLock::statusCmd()
{
    memset(commandBuffer, 0, 18);
    commandBuffer[0] = 0xee; // magic
    commandBuffer[1] = 0x02;
    commandBuffer[4] = 0x02;
    commandBuffer[16] = 0x02;
    return commandBuffer;
}

uint8_t AugustLock::getLockCode(LockAction action)
{
    uint8_t code = 0;
    switch (action)
    {
        case LOCK:
            code = 0x0b;
            break;
        case UNLOCK:
            code = 0x0a;
            break;
        case GET_STATUS:
            code = 0x02; 
            break;
        case TOGGLE_LOCK:
            break;        
    }

    return code;
}

const char* AugustLock::getLockActionStr(LockAction action)
{
    switch (action)
    {
        case LOCK:
            return "Lock";
        case UNLOCK:
            return "Unlock";
        case GET_STATUS:
            return "Get Status";
        case TOGGLE_LOCK:
            return "Toggle"; 
        default:
            return "";
    }
}

//
// BlueTooth Comms Callbacks
//

void AugustLock::_notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length)
{
    if (pRemoteCharacteristic->getUUID().equals(readHandshake))
    {
        // secure read, handshake response
        uint8_t *decryptMessageBytes = (uint8_t *)__builtin_alloca(length);
        decryptMessage(pData, decryptMessageBytes);

        AUGUST_LOG("Decrypt: %s", ConvertBytesToHex(decryptMessageBytes, length).c_str());

        // check
        if (decryptMessageBytes[0] == 0x02)
        {
            AUGUST_LOG("handshake pt1 complete");
            memcpy(handshakeKeyBytes, handshakeSessionKey, 8);
            memcpy(&handshakeKeyBytes[8], &decryptMessageBytes[4], 8);

            mbedtls_aes_setkey_enc(&lockCypher, handshakeKeyBytes, 128);

            // send init command
            uint8_t *cmd = BuildCommand(0x03);
            memcpy(&cmd[4], &handshakeSessionKey[8], 8);
            SecWriteChecksum(cmd);

            AUGUST_LOG("SEC_INITIALIZATION_COMMAND: %s", ConvertBytesToHex(cmd, 18).c_str());

            uint8_t secMessage[18];

            // encrypt
            encryptMessage(cmd, secMessage);

            AUGUST_LOG("Encrypted: %s", ConvertBytesToHex(secMessage, 18).c_str());

            delay(100);

            NimBLERemoteCharacteristic *pChr = pRemoteCharacteristic->getRemoteService()->getCharacteristic(writeHandshake);
            if (pChr->canWriteNoResponse())
            {
                AUGUST_LOG("Write no response SEC_INITIALIZATION_COMMAND");
                if (pChr->writeValue(secMessage, 18, false))
                {
                    AUGUST_LOG("Wrote new value to: %s", pChr->getUUID().toString().c_str());
                }
            }
            else
            {
                pClient->disconnect();
            }
        }
        else if (decryptMessageBytes[0] == 0x04)
        {
            AUGUST_LOG("handshake pt2 complete");
            AUGUST_LOG("Lock initialised");

            delay(100);
            switch (actionAfterSync)
            {
                case LOCK:
                case UNLOCK:
                    lockCommand(actionAfterSync);
                    break;
                case GET_STATUS:
                case TOGGLE_LOCK:
                    getStatus();
                    break;
            }
        }
        else
        {
            AUGUST_LOG("handshake failed");
        }
    }
}

void AugustLock::_secureLockCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length)
{
    AUGUST_LOG("Secure Lock: %s", ConvertBytesToHex(pData, length).c_str());

    uint8_t *decryptMessageBytes = (uint8_t *)__builtin_alloca(length);
    cbcDecryptMessage(pData, decryptMessageBytes);

    AUGUST_LOG("Decrypt: %s", ConvertBytesToHex(decryptMessageBytes, length).c_str());

    uint8_t status = decryptMessageBytes[8];

    if (status == 0x03)
    {
        connectCallback("Unlocked", false);
        if (actionAfterSync == TOGGLE_LOCK)
            lockCommand(LOCK);
        else
            closeConnection();
    }
    else if (status == 0x05)
    {
        connectCallback("Locked", true);
        if (actionAfterSync == TOGGLE_LOCK)
            lockCommand(UNLOCK);
        else
            closeConnection();
    }

    actionAfterSync = GET_STATUS;
}

//
// Crypto
//
void AugustLock::resetCrypto()
{
    std::vector<char> localBytes = HexToBytes(handshakeKey);
    memcpy(handshakeKeyBytes, localBytes.data(), 16);
    memset(iv, 0, sizeof(iv));
    memset(iv2, 0, sizeof(iv2));
    mbedtls_aes_init(&lockCypher);
}

void AugustLock::initSessionKey()
{
    // need 16 bytes random key;
    for (int i = 0; i < 4; i++)
    {
        uint32_t rand = esp_random();
        for(int j = 0; j < 4; j++)
        {
            handshakeSessionKey[i*4 + j] = rand >> (j*8);
        }
    }

    AUGUST_LOG("Session Key: %s", ConvertBytesToHex(handshakeSessionKey, 16).c_str());
}

void AugustLock::getHandshakeBytes(uint8_t *handshakeBytes)
{
    initSessionKey();

    // now build the command
    uint8_t *handshakeCommand = BuildCommand(0x01);
    memcpy(&handshakeCommand[4], handshakeSessionKey, 8);

    SecWriteChecksum(handshakeCommand);

    AUGUST_LOG("Handshake: %s", ConvertBytesToHex(handshakeCommand, 18).c_str());

    encryptMessage(handshakeCommand, handshakeBytes);

    AUGUST_LOG("Encrypted: %s", ConvertBytesToHex(handshakeBytes, 18).c_str());
}

void AugustLock::encryptMessage(uint8_t *plainText, uint8_t *encryptBuffer)
{
    mbedtls_aes_context aes;
    uint8_t messageChop[16];
    memcpy(encryptBuffer, plainText, 18);
    memcpy(messageChop, plainText, 16);
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char *)handshakeKeyBytes, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char *)messageChop, encryptBuffer);
    mbedtls_aes_free(&aes);
}

void AugustLock::decryptMessage(uint8_t *encryptedMessage, uint8_t *decryptBuffer)
{
    mbedtls_aes_context aes;
    uint8_t messageChop[16];
    memcpy(decryptBuffer, encryptedMessage, 18);
    memcpy(messageChop, encryptedMessage, 16);
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char *)handshakeKeyBytes, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char *)messageChop, decryptBuffer);
    mbedtls_aes_free(&aes);
}

void AugustLock::cbcEncryptMessage(uint8_t *plainText, uint8_t *encryptBuffer)
{
    uint8_t messageChop[16];
    memcpy(encryptBuffer, plainText, 18);
    memcpy(messageChop, plainText, 16);
    mbedtls_aes_crypt_cbc(&lockCypher, MBEDTLS_AES_ENCRYPT, 16, iv, (const unsigned char *)messageChop, (uint8_t *)encryptBuffer);
}

void AugustLock::cbcDecryptMessage(uint8_t *encryptedMessage, uint8_t *decryptBuffer)
{
    uint8_t messageChop[16];
    memcpy(decryptBuffer, encryptedMessage, 18);
    memcpy(messageChop, encryptedMessage, 16);
    mbedtls_aes_crypt_cbc(&lockCypher, MBEDTLS_AES_DECRYPT, 16, iv2, (const unsigned char *)messageChop, (uint8_t *)decryptBuffer);
}

uint32_t AugustLock::ReadUInt32LE(uint8_t *buffer, int startIndex)
{
    return *((uint32_t *)&buffer[startIndex]);
}

void AugustLock::WriteUInt32LE(uint8_t *buffer, uint value, int startIndex)
{
    *((uint32_t *)&buffer[startIndex]) = value;
}

uint32_t AugustLock::SecSecurityChecksum(uint8_t *buffer)
{
    return (0 - (ReadUInt32LE(buffer, 0x00) + ReadUInt32LE(buffer, 0x04) + ReadUInt32LE(buffer, 0x08))) >> 0;
}

void AugustLock::SecWriteChecksum(uint8_t *buffer)
{
    uint32_t cs = SecSecurityChecksum(buffer);
    WriteUInt32LE(buffer, cs, 0x0c);
}

uint8_t AugustLock::SimpleChecksum(uint8_t *buffer)
{
    uint32_t cs = 0;
    for (int i = 0; i < 18; i++)
    {
        cs = (cs + buffer[i]) & 0xff;
    }
    return (uint8_t)((-cs) & 0xff);
}

void AugustLock::WriteChecksum(uint8_t *buffer)
{
    buffer[3] = SimpleChecksum(buffer);
}