#ifndef NATIVE_TEST
#include <Arduino.h>
#include <Preferences.h>
#include "esp_system.h"
#include "esp_timer.h"
#endif

#include "PortalBoot.h"
#include "../Web3Log.h"

#ifndef NATIVE_TEST
static Preferences portalPrefs;
static esp_timer_handle_t bootCountClearTimer = nullptr;
static PortalBoot* bootCountClearInstance = nullptr;

static void bootCountClearTimerCallback(void* arg) {
    if (bootCountClearInstance && bootCountClearInstance->isInitialized()) {
        if (!bootCountClearInstance->isBootCountCleared()) {
            WEB3E_PRINTLN("[PortalBoot] Timer: 10s uptime reached — clearing boot_count");
            bootCountClearInstance->clearBootCountNow();
        }
    }
}
#endif

bool PortalBoot::begin() {
#ifndef NATIVE_TEST
    if (!portalPrefs.begin(PORTAL_BOOT_NVS_NAMESPACE, false)) {
        WEB3E_PRINTLN("ERROR: PortalBoot NVS begin failed");
        initialized = false;
        return false;
    }
#endif
    initialized = true;
    bootCountCleared = false;
    
#ifndef NATIVE_TEST
    bootCountClearInstance = this;
    if (bootCountClearTimer == nullptr) {
        const esp_timer_create_args_t timerArgs = {
            .callback = &bootCountClearTimerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "boot_count_clear",
            .skip_unhandled_events = false
        };
        esp_err_t err = esp_timer_create(&timerArgs, &bootCountClearTimer);
        if (err == ESP_OK) {
            err = esp_timer_start_once(bootCountClearTimer, PORTAL_BOOT_STABLE_MS * 1000ULL);
            if (err == ESP_OK) {
                WEB3E_PRINT("[PortalBoot] Timer armed: will clear boot_count after ");
                WEB3E_PRINT(PORTAL_BOOT_STABLE_MS);
                WEB3E_PRINTLN(" ms");
            } else {
                WEB3E_PRINTLN("[PortalBoot] WARNING: Failed to start timer");
            }
        } else {
            WEB3E_PRINTLN("[PortalBoot] WARNING: Failed to create timer");
        }
    }
#endif
    
    return readState();
}

bool PortalBoot::readState() {
    if (!initialized) return false;
#ifndef NATIVE_TEST
    stayPortal = portalPrefs.getUChar(PORTAL_BOOT_KEY_STAY_PORTAL, 0) != 0;
    bootCount = portalPrefs.getUChar(PORTAL_BOOT_KEY_BOOT_COUNT, 0);
#else
    stayPortal = false;
    bootCount = 0;
#endif
    return true;
}

bool PortalBoot::writeStayPortal(uint8_t value) {
    if (!initialized) return false;
#ifndef NATIVE_TEST
    size_t written = portalPrefs.putUChar(PORTAL_BOOT_KEY_STAY_PORTAL, value);
    if (written == 0) {
        if (portalPrefs.getUChar(PORTAL_BOOT_KEY_STAY_PORTAL, 0xFF) != value) {
            return false;
        }
    }
#endif
    stayPortal = (value != 0);
    return true;
}

bool PortalBoot::writeBootCount(uint8_t value) {
    if (!initialized) return false;
#ifndef NATIVE_TEST
    size_t written = portalPrefs.putUChar(PORTAL_BOOT_KEY_BOOT_COUNT, value);
    if (written == 0) {
        if (portalPrefs.getUChar(PORTAL_BOOT_KEY_BOOT_COUNT, 0xFF) != value) {
            return false;
        }
    }
#endif
    bootCount = value;
    return true;
}

bool PortalBoot::checkAndHandlePowerCycle() {
    if (!initialized) return shouldStayInPortal();

#ifndef NATIVE_TEST
    esp_reset_reason_t reason = esp_reset_reason();
    bool isRealPowerCycle = (reason == ESP_RST_POWERON || reason == ESP_RST_EXT);
    
    WEB3E_PRINT("[PortalBoot] Reset reason: ");
    WEB3E_PRINT((int)reason);
    WEB3E_PRINT(" (");
    WEB3E_PRINT(isRealPowerCycle ? "POWER/EXTERNAL" : "SOFTWARE/WATCHDOG");
    WEB3E_PRINTLN(")");
    
    if (!isRealPowerCycle) {
        WEB3E_PRINTLN("[PortalBoot] Not a real power cycle — boot_count unchanged");
        return shouldStayInPortal();
    }
#endif

    uint8_t next = bootCount;
    if (next < 255) {
        next++;
    }
    writeBootCount(next);

#ifndef NATIVE_TEST
    WEB3E_PRINT("[PortalBoot] Real power cycle: boot_count = ");
    WEB3E_PRINT((unsigned)bootCount);
    WEB3E_PRINT(" (threshold ");
    WEB3E_PRINT((unsigned)PORTAL_BOOT_TRIPLE_THRESHOLD);
    WEB3E_PRINTLN(")");
#endif

    if (bootCount >= PORTAL_BOOT_TRIPLE_THRESHOLD) {
#ifndef NATIVE_TEST
        WEB3E_PRINTLN("[PortalBoot] Triple power-cycle detected — forcing portal");
#endif
        setPortalReason(PORTAL_REASON_POWER_CYCLE);
        writeStayPortal(1);
        writeBootCount(0);
        bootCountCleared = true;
        return true;
    }

    return shouldStayInPortal();
}

bool PortalBoot::shouldStayInPortal() const {
    return stayPortal;
}

bool PortalBoot::setStayInPortal(bool stay) {
    return writeStayPortal(stay ? 1 : 0);
}

bool PortalBoot::clearStayInPortal() {
    return writeStayPortal(0);
}

void PortalBoot::clearBootCountIfStable(unsigned long uptimeMs) {
    if (!initialized || bootCountCleared) return;
    if (uptimeMs < PORTAL_BOOT_STABLE_MS) return;
    clearBootCountNow();
}

void PortalBoot::clearBootCountNow() {
    if (!initialized || bootCountCleared) return;
    if (bootCount == 0) {
        bootCountCleared = true;
        return;
    }
    writeBootCount(0);
    bootCountCleared = true;
}

uint8_t PortalBoot::getBootCount() const {
    return bootCount;
}

PortalReason PortalBoot::getPortalReason() const {
    if (!initialized) return PORTAL_REASON_NONE;
#ifndef NATIVE_TEST
    return (PortalReason)portalPrefs.getUChar("portal_reason", PORTAL_REASON_NONE);
#else
    return PORTAL_REASON_NONE;
#endif
}

bool PortalBoot::setPortalReason(PortalReason reason) {
    if (!initialized) return false;
#ifndef NATIVE_TEST
    size_t written = portalPrefs.putUChar("portal_reason", (uint8_t)reason);
    if (written == 0) {
        if (portalPrefs.getUChar("portal_reason", 0xFF) != (uint8_t)reason) {
            return false;
        }
    }
#endif
    return true;
}
