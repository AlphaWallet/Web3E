#ifndef RPC_PAIR_POLICY_H
#define RPC_PAIR_POLICY_H

/**
 * Round-robin index helpers for cached RPC load-sharing (Arduino-free).
 *
 * Web3::exec uses the current index for a request, may retry once on the other
 * node on transport failure, then advances to nextIndex for the following
 * request. Index lives in RAM only (starts at 0 after boot).
 */

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

#endif // RPC_PAIR_POLICY_H
