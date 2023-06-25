#include <Arduino.h>
#include <August.h>
#include <vector>
#include <string>
#include <esp_wifi.h>
#include <Web3.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <TcpBridge.h>

TaskHandle_t BluetoothTask;
TaskHandle_t Web3Task;

// LED pins
const int led1 = 2;
const int led2 = 4;


// SETUP Instructions: Note I will create a YouTube walkthrough for a very easy way to do this but it's not ready yet :)

// 1. Obtain Sepolia or Mumbai from the faucet if you don't have any (https://sepoliafaucet.com/). For convenience you can use AlphaWallet to hold the account key. There is
//    an alternative to this where you can use a JavaScript page viewer with any wallet to drive the TokenScript.

// Then, EITHER:

// 2. Use the Wizard at http://smarttokenlabs.duckdns.org/ (please note it is http, so you will get a warning - ensure you are using testnet on your AlphaWallet or metamask) 
//     which will create the NFT, deploy the TokenScript on IPFS and link to NFT via EIP-5169 and create the firmware for you. If you use this, skip to 5.

// OR

// 2. Create an NFT on the chain you are using. There are many guides on YouTube/Medium for how to do this. Copy/Paste the contract address in the "DOOR_CONTRACT" below.
// 3. Use MetaMask to create a new account. Copy the address into a notepad app like nano or notepad depending on your OS. Past the private key into "DEVICE_PRIVATE_KEY" below.
//     DO NOT use this account as your ethereum wallet!! This firmware does not leak the key but simply having a private key of value in plain text is really unsafe 
//     - you may accidentally commit it to github. Try not to commit the private key to github as someone could disrupt your comms if you do.
// 4. Create the TokenScript by editing this file; replace the 'origin contract' near the top with the DOOR_CONTRACT address, and replace both instances of the iot_address with 
//       the address from MetaMask created in step 3.

// 5. Obtain the August/Yale bluetooth codes for your lock. There are 2 ways to do this. For both you will require a rooted Android phone. 
//     a. Manually, Connect to you lock as normal, switch off WIFI on your phone and then open/close the lock a few times; this will force the app to create bluetooth credentials.
//        these credentials can be read from the private data area on the phone. Look in data/data/com.august.luna or dat/data/com.august.bennu. Go to shared_prefs and look for the file
//        PeripheralInfoCache. Within the file you'll find the Bluetooth address, the handshake key and the handshakekey index. Copy those into the augustLock object object below.
//     b. Use the app I wrote to pull these values automatically. Either build it yourself from https://github.com/JamesSmartCell/AugustLockCredentials or download from the PlayStore here:
//        https://play.google.com/store/apps/details?id=com.stormbird.augustcodereader it's open source so you can check it's not sending your lock codes to me!
// 6. Edit the TokenScript. Pull the file here:
// 7. Set up your wifi SSID and password below.
// 8. Open your account with the NFT in AlphaWallet, click on the NFT Token, you should see some additional verbs not just 'Transfer'. You should see 'Lock' and 'Unlock'.
//     These are provided via the TokenScript and act through the SmartLayer to operate the firmware with full ECDSA Authentication.

typedef struct
{
    const char *ssid;
    const char *password;
} WiFiCredentials;

std::vector<WiFiCredentials> wiFiCredentials{
    //{"wifi network 1", "12345678"},
    //{"wifi network 2", "12345678"}, 
    //{"wifi network 3", "12345678"},
};

AugustLock augustLock("78:9C:00:00:00:00", "00000000000000000000000000000000", 1); // Lock Bluetooth address, lock handshakeKey, lock handshakeKeyIndex                                      

#define DOOR_CONTRACT  "0x0000000000000000000000000000000000000000" // Contract address of an NFT you hold on the network you are using (default Sepolia)
#define DEVICE_PRIVATE_KEY "0000000000000000000000000000000000000000000000000000000000000000" // any private key - use MetaMask to create a new account; this will be the 
                                                                                              // Smart layer interface key. It does not have any value or blockchain transactions
                                                                                              // Paste the private key here and make a note of the associated address - you will use that
                                                                                              // Address in your TokenScript to communicate with this device via Smart Layer

//#define SPOOF_MAC_ADDRESS {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  //In case you need to connect to a network that requires a specific mac address to connect

void BlueToothService(void *pvParameters);
void Web3Service(void *pvParameters);
void updateChallenge();
void setupWifi();

const char *seedWords[] = {"Apples", "Oranges", "Grapes", "DragonFruit", "BreadFruit", "Pomegranate", "Aubergine", "Fungi", "Falafel", "Cryptokitty", "Kookaburra", "Elvis", "Koala", 0};

string currentChallenge;

TcpBridge *tcpConnection;

const char *apiRoute = "api/";

typedef enum DoorCommand
{
    cmd_none,
    cmd_startBlueTooth,
    cmd_disconnectBlueTooth,
    cmd_unlock,
    cmd_lock,
    cmd_query,
    cmd_statusReturn,
    cmd_end
} DoorCommand;

typedef enum ConnectionState
{
    DISCONNECTED,
    RECONNECT,
    WAIT_FOR_CONNECTION,
    CONNECTED,
    WAIT_FOR_COMMAND_COMPLETION,
    WAIT_FOR_DISCONNECT,
    DISCONNECT,
} ConnectionState;

enum LockComms
{
    lock_command,
    unlock_command,
    wait_for_command_complete,
    idle
};

enum APIRoutes
{
    api_unknown,
    api_getChallenge,
    api_checkSignature,
    api_checkSignatureLock,
    api_checkMarqueeSig,
    api_end
};

std::map<std::string, APIRoutes> s_apiRoutes;

void Initialize()
{
    s_apiRoutes["getChallenge"] = api_getChallenge;
    s_apiRoutes["checkSignature"] = api_checkSignature;
    s_apiRoutes["checkSignatureLock"] = api_checkSignatureLock;
    s_apiRoutes["end"] = api_end;
}

ConnectionState state = DISCONNECTED;
Web3 *web3;
KeyID *keyID;
LockComms lockStatus = idle;
DoorCommand command = cmd_none;
DoorCommand nextCmd = cmd_none;
long bluetoothScanTime;
long callTime = 0;
long infoTime = millis();
bool isLocked;
bool isConnected;
volatile long web3PulseTime;

void changeState(ConnectionState newState)
{
    Serial.print("New State: ");
    switch (newState)
    {
    case DISCONNECTED:
        Serial.println("Disconnected");
        break;

    case RECONNECT:
        Serial.println("Reconnect");
        break;

    case WAIT_FOR_CONNECTION:
        Serial.println("Wait for connection");
        break;

    case CONNECTED:
        Serial.println("Connected");
        break;

    case WAIT_FOR_COMMAND_COMPLETION:
        Serial.println("Wait for command completion");
        break;

    case DISCONNECT:
        Serial.println("Disconnect");
        break;

    case WAIT_FOR_DISCONNECT:
        Serial.println("Wait for disconnect");
        break;
    }

    state = newState;
}

boolean checkCommsTime(long &checkTime, long seconds, const char *message = "")
{
    if (checkTime > 0 && millis() > (checkTime + 1000 * seconds))
    {
        if (message[0] != 0)
        {
            Serial.println(message);
        }
        checkTime = millis();
        return true;
    }
    else
    {
        return false;
    }
}

bool equalsIgnoreCase(const string &a, const string &b)
{
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}

// These two callback decls required here for the case of mutiple instances of AugustLock.
// It would be required to switch them here based on client address. TODO: It may be possible to fold them into the AugustLock class
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    // If you have multiple locks you'll need to switch here using the service address: pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress()
    augustLock._notifyCB(pRemoteCharacteristic, pData, length);
}

void secureLockCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    augustLock._secureLockCallback(pRemoteCharacteristic, pData, length);
}

void connectCallback(const char *msg, bool status)
{
    Serial.print("Connect Callback: ");
    Serial.println(msg);
    isLocked = status;
}

void statusCallback(const char *msg, bool status)
{
    bluetoothScanTime = 0; //connection successful
    Serial.print("Status Callback: ");
    Serial.println(msg);

    callTime = millis();
    std::string serialMsg = "statusReturn:";
    serialMsg += msg;

    if (msg[0] == 'C')
    {
        isConnected = true;
    }
    else if (msg[0] == 'L')
    {
        isLocked = true;
    }
    else
    {
        isLocked = false;
    }

    changeState(CONNECTED);

    if (nextCmd != cmd_none)
    {
        Serial.print("Issue cached command: ");
        Serial.println(nextCmd);
        command = nextCmd;
        nextCmd = cmd_none;
    }
}

void disconnectCallback()
{
    Serial.println("BlueTooth disconnected");
    isConnected = false;
    changeState(DISCONNECTED);
    augustLock.blankClient();
    callTime = 0;
    bluetoothScanTime = 0;
}

bool QueryBalance(const char *contractAddr, std::string *userAddress)
{
    // transaction
    bool hasToken = false;
    Contract contract(web3, contractAddr);
    string func = "balanceOf(address)";
    string param = contract.SetupContractData(func.c_str(), userAddress);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    // break down the result
    uint256_t baseBalance = web3->getUint256(&result);

    if (baseBalance > 0)
    {
        hasToken = true;
        Serial.println("Has token");
    }

    return hasToken;
}

std::string handleTCPAPI(APIReturn *apiReturn)
{
    Serial.println(apiReturn->apiName.c_str());
    string address;

    switch (s_apiRoutes[apiReturn->apiName])
    {
    case api_getChallenge:
        command = cmd_startBlueTooth;
        Serial.println(currentChallenge.c_str());
        return currentChallenge;
    case api_checkSignature:
    {
        Serial.print("Sig: ");
        Serial.println(apiReturn->params["sig"].c_str());
        address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &currentChallenge);
        int unlockSeconds = strtol(apiReturn->params["openTime"].c_str(), NULL, 10);
        Serial.print("EC-Addr: ");
        Serial.println(address.c_str());
        boolean attestationValid = QueryBalance(DOOR_CONTRACT, &address);
        updateChallenge(); // generate a new challenge after each check
        if (attestationValid)
        {
            // lockStatus = unlock_command;
            command = cmd_unlock;
            return string("pass");
        }
        else
        {
            return string("fail");
        }
    }
    case api_checkSignatureLock:
    {
        Serial.print("Sig: ");
        Serial.println(apiReturn->params["sig"].c_str());
        address = Crypto::ECRecoverFromPersonalMessage(&apiReturn->params["sig"], &currentChallenge);
        int unlockSeconds = strtol(apiReturn->params["openTime"].c_str(), NULL, 10);
        Serial.print("EC-Addr: ");
        Serial.println(address.c_str());
        boolean hasToken = QueryBalance(DOOR_CONTRACT, &address);
        updateChallenge(); // generate a new challenge after each check
        if (hasToken)
        {
            command = cmd_lock;
            return string("pass");
        }
        else
        {
            return string("fail");
        }
    }
    case api_unknown:
    case api_end:
        break;
    }

    return string("");
}

void checkWeb3Comms()
{
    if ((millis() - web3PulseTime) > 3 * 60 * 1000) //more than 10 minutes since server comms, reboot the device
    {
        Serial.println("Restarting due to server comms failure ...");
        delay(10);
        ESP.restart();
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);

    // create a task that will be executed in the BlueToothService() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
        BlueToothService, /* Task function. */
        "BlueTooth",   /* name of task. */
        15000,     /* Stack size of task */
        NULL,      /* parameter of the task */
        1,         /* priority of the task */
        &BluetoothTask,    /* Task handle to keep track of created task */
        0);        /* pin task to core 0 */

    delay(500);

    // create a task that will be executed in the Web3Service() function, with priority 1 and executed on core 1
    xTaskCreatePinnedToCore(
        Web3Service, /* Task function. */
        "Web3",   /* name of task. */
        10000,     /* Stack size of task */
        NULL,      /* parameter of the task */
        1,         /* priority of the task */
        &Web3Task,    /* Task handle to keep track of created task */
        1);        /* pin task to core 1 */

    delay(500);

    Initialize();
}

void issueCommand(DoorCommand opcode, LockAction action)
{
    if (state == CONNECTED)
    {
        Serial.print("Issue Command: ");
        Serial.println(opcode);
        changeState(WAIT_FOR_COMMAND_COMPLETION);
        augustLock.lockCommand(action);
        callTime = millis();
    }
    else if (state == DISCONNECTED)
    {
        nextCmd = opcode;
        changeState(RECONNECT);
    }
    else
    {
        nextCmd = opcode;
    }
}

void checkBlueToothStatus()
{
    switch (command)
    {
    case cmd_none:
        break;
    case cmd_startBlueTooth:
        command = cmd_none;
        Serial.println("State: cmd_startBlueTooth");
        Serial.println("Starting Bluetooth connection");
        if (state == DISCONNECTED)
            changeState(RECONNECT);
        break;
    case cmd_disconnectBlueTooth:
        command = cmd_none;
        Serial.println("State: cmd_disconnectBlueTooth");
        if (state == CONNECTED)
        {
            changeState(DISCONNECT);
        }
        else
        {
            nextCmd = command;
        }
        break;
    case cmd_unlock:
        command = cmd_none;
        Serial.println("State: cmd_unlock");
        issueCommand(command, UNLOCK);
        break;
    case cmd_lock:
        command = cmd_none;
        Serial.println("State: cmd_lock");
        issueCommand(command, LOCK);
        break;
    case cmd_query:
        command = cmd_none;
        issueCommand(command, GET_STATUS);
        break;
    case cmd_statusReturn:
        command = cmd_none;
        // shouldn't see this on this end
        break;
    default:
        command = cmd_none;
        Serial.println("State: cmd_none");
        Serial.print("Unknown command: ");
        Serial.println(command);
        break;
    }

    augustLock.checkStatus();
}

void checkState()
{
    switch (state)
    {
    case DISCONNECTED:
        // wait for instruction
        if (bluetoothScanTime > 0 && (millis() - bluetoothScanTime) > 5 * 60 * 1000)
        {
            Serial.println("Timeout BlueTooth Scan. Stop scanning and wait for user action to scan again");
            augustLock.stopScanning();
            bluetoothScanTime = 0;
        }
        break;

    case RECONNECT:
        checkCommsTime(infoTime, 20, "Has connection");
        augustLock.lockAction(ESTABLISH_CONNECTION);
        bluetoothScanTime = millis();
        callTime = millis();
        changeState(WAIT_FOR_CONNECTION);
        nextCmd = cmd_query;
        break;

    case WAIT_FOR_CONNECTION:
        checkCommsTime(infoTime, 20, "Wait for link");
        // TODO: Add timeout
        break;

    case CONNECTED:
        // link established, get status
        checkCommsTime(infoTime, 20, "Link established");
        if (checkCommsTime(callTime, 40))
        {
            // timeout if no call received for 40 seconds
            Serial.println("Connection timeout, close");
            changeState(DISCONNECT);
        }
        break;

    case WAIT_FOR_COMMAND_COMPLETION:
        checkCommsTime(infoTime, 20, "Wait for call completion ...");
        // TODO: Timeout
        break;

    case DISCONNECT:
        augustLock.closeConnection();
        changeState(WAIT_FOR_DISCONNECT);
        break;

    case WAIT_FOR_DISCONNECT:
        if (checkCommsTime(infoTime, 5))
        {
            Serial.println("Connected: Wait for disconnect");
            Serial.print("Lock is ");
            if (isLocked)
                Serial.println("Locked");
            else
                Serial.println("Unlocked");
        }
        // TODO: Timeout
        break;
    }
}

// BlueTooth comms with August / Yale smartlock
void BlueToothService(void *pvParameters)
{
    Serial.print("Task1 running on core ");
    Serial.println(xPortGetCoreID());

    delay(3000);

    Serial.println("Try BlueTooth");

    // Start listening on Bluetooth
    augustLock.init();
    augustLock.connect(&statusCallback, &notifyCB, &secureLockCallback, &disconnectCallback);
    int ledState = HIGH;
    int ledStateCount = 0;
    bluetoothScanTime = millis();

    for (;;)
    {
        delay(250);
        if (ledStateCount % (isLocked ? 8 : 2) == 0)
        {
            ledState = (ledState == HIGH) ? LOW : HIGH;
            ledStateCount = 0;
        }

        ledStateCount++;

        digitalWrite(led1, ledState);

        checkBlueToothStatus();
        checkState();
        checkWeb3Comms(); // watchdog to check we have regular Web3 Comms
    }
}

// Web3Service: wait for commands from the Smart Layer / TokenScript
void Web3Service(void *pvParameters)
{
    Serial.print("Task2 running on core ");
    Serial.println(xPortGetCoreID());

    setupWifi();

    web3 = new Web3(SEPOLIA_ID);
    keyID = new KeyID(web3, DEVICE_PRIVATE_KEY);
    updateChallenge();

    tcpConnection = new TcpBridge();
    tcpConnection->setKey(keyID, web3);
    tcpConnection->startConnection();

#ifdef SPOOF_MAC_ADDRESS
    uint8_t spoofMacAddress[] = SPOOF_MAC_ADDRESS;
    esp_wifi_set_mac(WIFI_IF_STA, &spoofMacAddress[0]);
#endif

    web3PulseTime = millis();

    for(;;)
    {
        delay(500);
        setupWifi();
        tcpConnection->checkClientAPI(&handleTCPAPI);
        web3PulseTime = tcpConnection->getLastCommsTime();
    }
}

void loop()
{
    // NOP, we are assigning two threads explicitly:
    // Thread 1: BlueTooth comms with smartlock
    // Thread 2: Web3 comms with smartlayer
}

void updateChallenge()
{
    // generate a new challenge
    int size = 0;
    while (seedWords[size] != 0)
    {
        size++;
    }

    Serial.println(size);
    char buffer[32];

    int seedIndex = random(0, size);
    currentChallenge = seedWords[seedIndex];
    currentChallenge += "-";
    long challengeVal = random32();
    currentChallenge += itoa(challengeVal, buffer, 16);

    Serial.print("Challenge: ");
    Serial.println(currentChallenge.c_str());
}

bool wifiConnect(const char *ssid, const char *password)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    if (WiFi.status() != WL_CONNECTED)
    {
        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
        WiFi.begin(ssid, password);
    }

    int wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 20)
    {
        for (int i = 0; i < 500; i++)
        {
            delay(1);
        }
        Serial.print(".");
        wificounter++;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("-");
        return false;
    }
    else
    {
        return true;
    }
}

void setupWifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    delay(100);

    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    bool connected = false;
    int index = 0;

    while (!connected)
    {
        connected = wifiConnect(wiFiCredentials[index].ssid, wiFiCredentials[index].password);
        if (++index > wiFiCredentials.size())
        {
            break;
        }
    }

    if (!connected)
    {
        Serial.println("Restarting ...");
        ESP.restart(); // targetting 8266 & Esp32 - you may need to replace this
    }

    esp_wifi_set_max_tx_power(78); // save a little power if your unit is near the router. If it's located away then use 78 - max
    delay(10);

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}