/**
 * Minimal stubs so Trezor ecdsa.o links on host without Arduino / TinyMT.
 * Not used by protocol verify paths under --gc-sections, but kept for safety.
 */
#include <stdint.h>
#include <string.h>

unsigned long millis(void) { return 1; }
unsigned long micros(void) { return 1; }

uint32_t random32(void) { return 0xC0FFEEu; }

void random_buffer(uint8_t *buf, size_t len) {
    /* Deterministic filler for unused sign paths in native tests */
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(0xA5 ^ (uint8_t)i);
    }
}
