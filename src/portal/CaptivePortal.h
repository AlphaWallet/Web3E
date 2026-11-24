#pragma once

#include <stdbool.h>
#include "PortalBoot.h"

#ifndef AP_SSID_PREFIX
#define AP_SSID_PREFIX "ESP32-Setup"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD ""
#endif

#ifndef APP_NAME
#define APP_NAME "Web3E-Device"
#endif

bool captivePortalRun(PortalBoot* portalBoot, const char* deviceAddress);
