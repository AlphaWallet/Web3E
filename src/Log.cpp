//
// Created by Okada, Takahiro on 2018/02/04.
//

#include "Log.h"
#include <Arduino.h>
#define USE_SERIAL Serial

void Log::print(const char* s) {
#ifdef DEBUGLOG
        USE_SERIAL.print(s);
#endif
}
void Log::println(const char* s) {
#ifdef DEBUGLOG
        USE_SERIAL.println(s);
#endif
}