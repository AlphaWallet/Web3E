#ifndef RPC_PAIR_POLICY_H
#define RPC_PAIR_POLICY_H

#include <stdint.h>

/**
 * Round-robin + blocked-node policy helpers for cached RPC pairs (Arduino-free).
 *
 * Web3::exec uses the current index for a request, may retry once on the other
 * node on transport failure, then advances to nextIndex for the following
 * request. Index lives in RAM only (starts at 0 after boot).
 *
 * After N consecutive connect/TLS failures on one node while the other is
 * succeeding, the device may request a replacement pair from uninode (rate-limited).
 */

#ifndef RPC_PAIR_BLOCK_FAILURE_THRESHOLD
#define RPC_PAIR_BLOCK_FAILURE_THRESHOLD 3
#endif

#ifndef RPC_PAIR_REPLACEMENT_MIN_INTERVAL_MS
#define RPC_PAIR_REPLACEMENT_MIN_INTERVAL_MS (30UL * 60UL * 1000UL) /* 30 min */
#endif

#ifndef RPC_PAIR_FEEDBACK_MAX_URLS
#define RPC_PAIR_FEEDBACK_MAX_URLS 8
#endif

/** mbedtls SSL invalid record — often plaintext HTTP during TLS (SNI filter). */
#ifndef RPC_PAIR_MBEDTLS_INVALID_RECORD
#define RPC_PAIR_MBEDTLS_INVALID_RECORD (-29184)
#endif

/** Field-observed companion code when AiProtection intercepts TLS. */
#ifndef RPC_PAIR_MBEDTLS_INTERCEPT_ALT
#define RPC_PAIR_MBEDTLS_INTERCEPT_ALT (-80)
#endif

/** Next primary index after a request that started at startIndex. */
inline int rpcPairNextIndex(int startIndex, int count) {
    if (count <= 1) {
        return 0;
    }
    if (startIndex < 0) {
        startIndex = 0;
    }
    return (startIndex + 1) % count;
}

/** Retry index for the same request after a transport failure on startIndex. */
inline int rpcPairRetryIndex(int startIndex, int count) {
    return rpcPairNextIndex(startIndex, count);
}

/**
 * True when a node should be treated as router-blocked: enough consecutive
 * connect/TLS failures while the peer node has been succeeding.
 */
inline bool rpcPairShouldMarkBlocked(int consecutiveFailures,
                                     int otherConsecutiveSuccesses,
                                     int threshold = RPC_PAIR_BLOCK_FAILURE_THRESHOLD) {
    if (threshold < 1) threshold = 1;
    return consecutiveFailures >= threshold && otherConsecutiveSuccesses > 0;
}

/** Rollover-safe rate limit for uninode replacement requests. */
inline bool rpcPairCanRequestReplacement(uint32_t nowMs,
                                         uint32_t lastRequestMs,
                                         bool everRequested,
                                         uint32_t minIntervalMs = RPC_PAIR_REPLACEMENT_MIN_INTERVAL_MS) {
    if (!everRequested) return true;
    return (nowMs - lastRequestMs) >= minIntervalMs;
}

/** True if mbedtls lastError indicates likely SNI / TLS interception. */
inline bool rpcPairLooksLikeTlsIntercept(int mbedtlsLastError) {
    return mbedtlsLastError == RPC_PAIR_MBEDTLS_INVALID_RECORD
        || mbedtlsLastError == RPC_PAIR_MBEDTLS_INTERCEPT_ALT;
}

/**
 * True if a peek buffer starts with an HTTP response / request line
 * (router returning plaintext on :443).
 */
inline bool rpcPairLooksLikePlaintextHttp(const char* buf, int len) {
    if (!buf || len < 4) return false;
    // "HTTP/" status or "GET " / "POST " etc. from a captive/filter page
    if (len >= 5 && buf[0] == 'H' && buf[1] == 'T' && buf[2] == 'T' && buf[3] == 'P' && buf[4] == '/') {
        return true;
    }
    if (len >= 4 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T' && buf[3] == ' ') {
        return true;
    }
    if (len >= 5 && buf[0] == 'P' && buf[1] == 'O' && buf[2] == 'S' && buf[3] == 'T' && buf[4] == ' ') {
        return true;
    }
    return false;
}

#endif // RPC_PAIR_POLICY_H
