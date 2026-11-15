#ifndef WEB3E_LOG_H
#define WEB3E_LOG_H

// WEB3E_RELEASE (from sketch web3e_flags.h / -DWEB3E_RELEASE) strips logging
// at preprocess time: args are discarded, so strings and side-effect-free
// expressions like ConvertBytesToHex(...).c_str() never reach the compiler.
#ifndef WEB3E_RELEASE
#define WEB3E_PRINT(...) Serial.print(__VA_ARGS__)
#define WEB3E_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define WEB3E_PRINT(...) ((void)0)
#define WEB3E_PRINTLN(...) ((void)0)
#endif

#endif
