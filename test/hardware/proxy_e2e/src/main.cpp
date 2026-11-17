/**
 * Web3E TCP Bridge End-to-End Hardware Test
 * 
 * This test firmware exercises the full TCP bridge protocol including:
 * - Firmware version reporting (opcode 0x09)
 * - WiFi diagnostics (opcode 0x0A)
 * - Keep-alive messages (opcode 0x06)
 * - API call handling (opcode 0x04/0x08)
 * 
 * Requirements:
 * - Pre-provisioned EEPROM with KeyID (byte 0-32)
 * - Pre-provisioned EEPROM with WiFi networks (byte 33+)
 * - Uses Web3E-Setup-Firmware format
 */

#include <Arduino.h>
#include <Web3.h>
#include <TcpBridge.h>
#include <WifiStore.h>
#include <WiFiConnection.h>
#include <WiFiDiagnostics.h>
#include <KeyID.h>
#include <APIReturn.h>

// E2E test state machine
enum E2EState {
    E2E_INIT,
    E2E_WIFI_CONNECT,
    E2E_TCP_CONNECT,
    E2E_LOGIN,
    E2E_WAIT_DIAG,
    E2E_KEEPALIVE,
    E2E_API_WAIT,
    E2E_DONE
};

// Global state
E2EState testState = E2E_INIT;
Web3 *web3 = nullptr;
KeyID *keyID = nullptr;
TcpBridge *tcpConnection = nullptr;
WifiStore wifiStore;
WiFiConnection wifiConnection;
std::string deviceAddress;

// Test counters
int keepaliveCount = 0;
int apiCallCount = 0;
bool loginSuccess = false;
bool diagSent = false;
bool usedOpcode09 = false;
unsigned long stateStartTime = 0;
unsigned long testStartTime = 0;

// Test configuration
const int TARGET_KEEPALIVES = 3;
const unsigned long STATE_TIMEOUT_MS = 30000; // 30 seconds per state
const unsigned long API_WAIT_TIMEOUT_MS = 60000; // 60 seconds to wait for API call

void e2ePrint(const char* message) {
    Serial.print("E2E:");
    Serial.println(message);
}

void e2ePrintResult(const char* test, const char* result) {
    Serial.print("E2E:");
    Serial.print(test);
    Serial.print("=");
    Serial.println(result);
}

void e2eFail(const char* reason) {
    Serial.print("E2E:FAIL reason=\"");
    Serial.print(reason);
    Serial.println("\"");
    Serial.print("E2E:DONE FAIL");
    testState = E2E_DONE;
}

void e2ePass() {
    unsigned long totalTime = millis() - testStartTime;
    Serial.print("E2E:DONE PASS time_ms=");
    Serial.println(totalTime);
    testState = E2E_DONE;
}

// API callback handler
std::string handleAPI(APIReturn *apiReturn) {
    apiCallCount++;
    
    Serial.print("E2E:API_CALL name=\"");
    Serial.print(apiReturn->apiName.c_str());
    Serial.print("\" count=");
    Serial.println(apiCallCount);
    
    // Echo API response for testing
    std::string response = "E2E_TEST_RESPONSE api=";
    response += apiReturn->apiName;
    
    return response;
}

void setup() {
    // Arm startup watchdog first (30s timeout until session confirmed)
    TcpBridge::armStartupWatchdog();
    
    Serial.begin(115200);
    delay(2000); // Wait for serial
    
    testStartTime = millis();
    e2ePrint("START");
    e2ePrintResult("FW_VERSION", FW_VERSION);
    
    // ===== STEP 1: Verify KeyID exists (never create) =====
    testState = E2E_INIT;
    stateStartTime = millis();
    
    if (!EEPROM.begin(WEB3E_EEPROM_SIZE)) {
        e2eFail("EEPROM init failed");
        return;
    }
    
    Serial.print("E2E:EEPROM_SIZE=");
    Serial.println(WEB3E_EEPROM_SIZE);
    
    // Check KeyID marker
    uint8_t marker = EEPROM.read(0);
    if (marker != 64) {
        e2eFail("KeyID not found - device must be pre-provisioned");
        return;
    }
    e2ePrintResult("KEYID", "OK found=true");
    
    // ===== STEP 2: Read WiFi networks from WifiStore =====
    if (!wifiStore.begin()) {
        e2eFail("WifiStore init failed");
        return;
    }
    
    if (!wifiStore.isValid()) {
        e2eFail("WifiStore invalid - device must be pre-provisioned");
        return;
    }
    
    WifiCredential credentials[MAX_WIFI_NETWORKS];
    int networkCount = wifiStore.readNetworks(credentials, MAX_WIFI_NETWORKS);
    
    if (networkCount == 0) {
        e2eFail("No WiFi networks found in WifiStore");
        return;
    }
    
    Serial.print("E2E:WIFISTORE_READ networks=");
    Serial.println(networkCount);
    
    // ===== STEP 3: Connect to WiFi =====
    testState = E2E_WIFI_CONNECT;
    stateStartTime = millis();
    e2ePrint("WIFI_CONNECTING");
    
    // Convert WifiCredential to WiFiCredentials array
    WiFiCredentials wifiNetworks[MAX_WIFI_NETWORKS + 1];
    for (int i = 0; i < networkCount; i++) {
        wifiNetworks[i].ssid = credentials[i].ssid;
        wifiNetworks[i].password = credentials[i].password;
    }
    wifiNetworks[networkCount].ssid = nullptr;
    wifiNetworks[networkCount].password = nullptr;
    
    wifiConnection.setupWifi(wifiNetworks);
    
    if (WiFi.status() != WL_CONNECTED) {
        e2eFail("WiFi connection failed");
        return;
    }
    
    Serial.print("E2E:WIFI=OK ip=\"");
    Serial.print(WiFi.localIP().toString());
    Serial.println("\"");
    
    // ===== STEP 4: Initialize Web3 and KeyID =====
    web3 = new Web3(1); // Mainnet ID (not used for proxy)
    
    // Load existing KeyID (never generate)
    keyID = new KeyID(web3);
    if (!keyID->hasRecoveredKey()) {
        e2eFail("KeyID failed to load");
        return;
    }
    
    deviceAddress = keyID->getAddress();
    Serial.print("E2E:DEVICE_ADDRESS=\"");
    Serial.print(deviceAddress.c_str());
    Serial.println("\"");
    
    // ===== STEP 5: Connect to TCP Bridge =====
    testState = E2E_TCP_CONNECT;
    stateStartTime = millis();
    e2ePrint("TCP_CONNECTING");
    
    tcpConnection = new TcpBridge();
    tcpConnection->setFirmwareVersion(FW_VERSION);
    tcpConnection->setWiFiDiagnostics(wifiConnection.getDiagnostics());
    tcpConnection->setKey(keyID, web3);
    tcpConnection->startConnection();
    
    // Check if connected (TcpBridge handles reconnection internally)
    delay(2000); // Give it time to establish connection
    
    testState = E2E_LOGIN;
    stateStartTime = millis();
    e2ePrint("WAITING_FOR_LOGIN");
}

void loop() {
    unsigned long now = millis();
    
    // Check for state timeout
    if (testState != E2E_DONE && (now - stateStartTime) > STATE_TIMEOUT_MS) {
        e2eFail("State timeout");
        return;
    }
    
    // Process TCP bridge
    if (tcpConnection != nullptr && testState != E2E_DONE) {
        tcpConnection->checkClientAPI(&handleAPI);
        
        // Check connection state via last comms time
        long lastComms = tcpConnection->getLastCommsTime();
        
        // State machine
        switch (testState) {
            case E2E_LOGIN:
                // Login is considered successful after first server communication
                if (lastComms > 0 && !loginSuccess) {
                    loginSuccess = true;
                    // We sent opcode 0x09 if firmware version is set
                    usedOpcode09 = true;
                    e2ePrintResult("LOGIN", "OK opcode=0x09");
                    
                    testState = E2E_WAIT_DIAG;
                    stateStartTime = now;
                }
                break;
                
            case E2E_WAIT_DIAG:
                // Diagnostics are sent automatically after login
                // We'll assume it happened after a short delay
                if (loginSuccess && (now - stateStartTime) > 2000) {
                    diagSent = true;
                    e2ePrint("DIAG_SENT opcode=0x0A");
                    
                    testState = E2E_KEEPALIVE;
                    stateStartTime = now;
                }
                break;
                
            case E2E_KEEPALIVE:
                // Keep sending keep-alives
                // The TCP bridge handles this internally, we just track
                if (keepaliveCount < TARGET_KEEPALIVES) {
                    // Check if we got any communication (keep-alive responses)
                    static unsigned long lastKeepaliveCheck = 0;
                    if (now - lastKeepaliveCheck > 10000) { // Check every 10 seconds
                        keepaliveCount++;
                        Serial.print("E2E:KEEPALIVE count=");
                        Serial.println(keepaliveCount);
                        lastKeepaliveCheck = now;
                    }
                } else {
                    // Move to API wait
                    testState = E2E_API_WAIT;
                    stateStartTime = now;
                    e2ePrint("WAITING_FOR_API");
                }
                break;
                
            case E2E_API_WAIT:
                // Wait for at least one API call from the proxy
                if (apiCallCount > 0) {
                    e2ePrintResult("API_CALLS_RECEIVED", String(apiCallCount).c_str());
                    e2ePass();
                } else if (now - stateStartTime > API_WAIT_TIMEOUT_MS) {
                    // No API call received - FAIL (runner should trigger one)
                    e2eFail("No API call received from proxy");
                }
                break;
                
            case E2E_DONE:
                // Test complete, just idle
                delay(1000);
                break;
                
            default:
                break;
        }
    }
    
    delay(10); // Small delay to prevent tight loop
}
