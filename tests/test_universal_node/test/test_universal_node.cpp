/**
 * Host-side unit tests for universal-node protocol (canonical JSON,
 * request message construction, response verification).
 *
 * Fixed vectors match JamesSmartCell/universal-node docs/protocol.md /
 * tests/protocol.test.ts using Anvil account #0 / #1 keys (NOT real secrets):
 *   Device: 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
 *           → 0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
 *   Node:   0x59c6995e998f97a5a0044966f0945389dc9e86dae88c7a8412f4603b6b78690d
 *           → 0x70997970C51812dc3A010C7d01b50e0d17dc79C8
 */

#include <unity.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <UniversalNodeProtocol.h>
#include <RpcPairPolicy.h>

using namespace UniversalNodeProtocol;
using namespace std;

// --- Canonical JSON ---

void test_canonical_sorts_object_keys() {
    TEST_ASSERT_EQUAL_STRING("{\"a\":2,\"b\":1}", canonicalJson("{\"b\":1,\"a\":2}").c_str());
}

void test_canonical_array_and_literals() {
    TEST_ASSERT_EQUAL_STRING("[1,\"x\",true,null]", canonicalJson("[1, \"x\", true, null]").c_str());
}

void test_canonical_nested() {
    TEST_ASSERT_EQUAL_STRING("{\"y\":[],\"z\":{\"a\":0,\"b\":1}}",
                             canonicalJson("{\"z\":{\"b\":1,\"a\":0},\"y\":[]}").c_str());
}

void test_canonical_empty_array() {
    TEST_ASSERT_EQUAL_STRING("[]", canonicalJson("[]").c_str());
    TEST_ASSERT_EQUAL_STRING("[]", paramsEmpty().c_str());
}

void test_params_eth_call_key_order() {
    // data before to (UTF-8 code unit sort)
    string p = paramsEthCall(
        "0x0000000000000000000000000000000000000001",
        "0x70a08231000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266",
        "latest");
    TEST_ASSERT_EQUAL_STRING(
        "[{\"data\":\"0x70a08231000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266\","
        "\"to\":\"0x0000000000000000000000000000000000000001\"},\"latest\"]",
        p.c_str());
}

// --- Request message ---

static const char* kRequestParamsCanon =
    "[{\"data\":\"0x70a08231000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266\","
    "\"to\":\"0x0000000000000000000000000000000000000001\"},\"latest\"]";

static const char* kParamsHash =
    "0x7c42a45b0ed44aadd82590c706663fa11733b51ad87e644eaef703764c8c6cf3";

static const char* kRequestMessage =
    "universal-node.request.v1\n"
    "chainId:1\n"
    "method:eth_call\n"
    "params:0x7c42a45b0ed44aadd82590c706663fa11733b51ad87e644eaef703764c8c6cf3\n"
    "timestamp:1700000000\n"
    "nonce:0x00112233445566778899aabbccddeeff";

static const char* kRequestHash =
    "0xb27afdad8c4099d404dbe4ca6d10e67c3812ba12b544882a1302be5409d66688";

static const char* kRequestSignature =
    "0xb027b0483f039902982122cd803ebab4b47ca7d88a8758ce0881d13c473dff3f"
    "516e3dab31fa1a2e71d6a883c6f9a4454998025c0db86df2dd5d66c9aacabce51c";

void test_hash_params_matches_vector() {
    TEST_ASSERT_EQUAL_STRING(kParamsHash, hashParams(kRequestParamsCanon).c_str());
}

void test_build_request_signing_message() {
    string msg = buildRequestSigningMessage(
        1, "eth_call", kRequestParamsCanon, 1700000000LL,
        "0x00112233445566778899aabbccddeeff");
    TEST_ASSERT_EQUAL_STRING(kRequestMessage, msg.c_str());
    TEST_ASSERT_FALSE(msg.empty());
    TEST_ASSERT_TRUE(msg[msg.size() - 1] != '\n');
    TEST_ASSERT_TRUE(msg.find("\n\n") == string::npos);
}

void test_compute_request_hash() {
    string msg = buildRequestSigningMessage(
        1, "eth_call", kRequestParamsCanon, 1700000000LL,
        "0x00112233445566778899aabbccddeeff");
    TEST_ASSERT_EQUAL_STRING(kRequestHash, computeRequestHash(msg).c_str());
}

void test_recover_device_from_request_signature() {
    string recovered = recoverPersonalSigner(kRequestMessage, kRequestSignature);
    TEST_ASSERT_EQUAL_STRING("0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266", recovered.c_str());
}

// --- Response verification ---

static const char* kBalanceParamsCanon =
    "[\"0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266\",\"latest\"]";

static const char* kBalanceRequestHash =
    "0x8771015e767cf21c27a4ed84c638a00bf9c96124df70d1489b45e254b742ce8e";

static const char* kResultCanon =
    "\"0x0000000000000000000000000000000000000000000000000de0b6b3a7640000\"";

static const char* kResultHash =
    "0x218e71778e456594c79d850430312df75a55be1913929e6ec5dcb12775e05697";

static const char* kResponseMessage =
    "universal-node.response.v1\n"
    "device:0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266\n"
    "nonce:0xaabbccddeeff00112233445566778899\n"
    "chainId:8453\n"
    "requestHash:0x8771015e767cf21c27a4ed84c638a00bf9c96124df70d1489b45e254b742ce8e\n"
    "blockNumber:12345678\n"
    "timestamp:1700000001\n"
    "result:0x218e71778e456594c79d850430312df75a55be1913929e6ec5dcb12775e05697";

static const char* kResponseSignature =
    "0x7034f2cff5ead8e011a8f0bd1525c0e50564074f93d6ccbff8b3152af78c0750"
    "155dbf2cc0a08f6fc837395f5bde7c3ae5967bcd6c1e16437d4e5544a85d68bc1b";

static const char* kNodeSigner = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8";

void test_balance_params_and_request_hash() {
    string built = paramsEthGetBalance("0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266", "latest");
    TEST_ASSERT_EQUAL_STRING(kBalanceParamsCanon, built.c_str());

    string msg = buildRequestSigningMessage(
        8453, "eth_getBalance", built, 1700000000LL,
        "0xaabbccddeeff00112233445566778899");
    TEST_ASSERT_EQUAL_STRING(kBalanceRequestHash, computeRequestHash(msg).c_str());
}

void test_hash_result_and_response_message() {
    TEST_ASSERT_EQUAL_STRING(kResultHash, hashResult(kResultCanon).c_str());

    string msg = buildResponseSigningMessage(
        "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266",
        "0xaabbccddeeff00112233445566778899",
        8453,
        kBalanceRequestHash,
        "12345678",
        1700000001LL,
        kResultCanon);
    TEST_ASSERT_EQUAL_STRING(kResponseMessage, msg.c_str());
}

void test_recover_node_from_response_signature() {
    string recovered = recoverPersonalSigner(kResponseMessage, kResponseSignature);
    TEST_ASSERT_EQUAL_STRING("0x70997970c51812dc3a010c7d01b50e0d17dc79c8", recovered.c_str());
}

void test_verify_response_accepts_valid() {
    // Minimal response body matching protocol field names
    string body = string("{")
        + "\"result\":\"0x0000000000000000000000000000000000000000000000000de0b6b3a7640000\","
        + "\"blockNumber\":\"12345678\","
        + "\"chainId\":8453,"
        + "\"timestamp\":1700000001,"
        + "\"requestHash\":\"" + string(kBalanceRequestHash) + "\","
        + "\"signature\":\"" + string(kResponseSignature) + "\","
        + "\"signer\":\"" + string(kNodeSigner) + "\"}";

    string resultCanon;
    string blockNumber;
    VerifyStatus st = verifyResponse(
        body,
        "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266",
        "0xaabbccddeeff00112233445566778899",
        8453,
        kBalanceRequestHash,
        kNodeSigner,
        1700000001LL,
        60,
        &resultCanon,
        &blockNumber);

    TEST_ASSERT_EQUAL(VERIFY_OK, st);
    TEST_ASSERT_EQUAL_STRING(kResultCanon, resultCanon.c_str());
    TEST_ASSERT_EQUAL_STRING("12345678", blockNumber.c_str());
}

void test_verify_response_rejects_wrong_signer() {
    string body = string("{")
        + "\"result\":\"0x0000000000000000000000000000000000000000000000000de0b6b3a7640000\","
        + "\"blockNumber\":\"12345678\","
        + "\"chainId\":8453,"
        + "\"timestamp\":1700000001,"
        + "\"requestHash\":\"" + string(kBalanceRequestHash) + "\","
        + "\"signature\":\"" + string(kResponseSignature) + "\","
        + "\"signer\":\"" + string(kNodeSigner) + "\"}";

    VerifyStatus st = verifyResponse(
        body,
        "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266",
        "0xaabbccddeeff00112233445566778899",
        8453,
        kBalanceRequestHash,
        "0xF09b881B849B50D9A3d96b7Aed50f34b5F585A30", // production pin ≠ Anvil #1
        1700000001LL,
        60,
        nullptr,
        nullptr);

    TEST_ASSERT_EQUAL(VERIFY_SIGNER_MISMATCH, st);
}

void test_verify_response_rejects_bad_request_hash() {
    string body = string("{")
        + "\"result\":\"0x0000000000000000000000000000000000000000000000000de0b6b3a7640000\","
        + "\"blockNumber\":\"12345678\","
        + "\"chainId\":8453,"
        + "\"timestamp\":1700000001,"
        + "\"requestHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\","
        + "\"signature\":\"" + string(kResponseSignature) + "\"}";

    VerifyStatus st = verifyResponse(
        body,
        "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266",
        "0xaabbccddeeff00112233445566778899",
        8453,
        kBalanceRequestHash,
        kNodeSigner,
        1700000001LL,
        60,
        nullptr,
        nullptr);

    TEST_ASSERT_EQUAL(VERIFY_REQUEST_HASH_MISMATCH, st);
}

void test_verify_response_rejects_timestamp_skew() {
    string body = string("{")
        + "\"result\":\"0x0000000000000000000000000000000000000000000000000de0b6b3a7640000\","
        + "\"blockNumber\":\"12345678\","
        + "\"chainId\":8453,"
        + "\"timestamp\":1700000001,"
        + "\"requestHash\":\"" + string(kBalanceRequestHash) + "\","
        + "\"signature\":\"" + string(kResponseSignature) + "\"}";

    VerifyStatus st = verifyResponse(
        body,
        "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266",
        "0xaabbccddeeff00112233445566778899",
        8453,
        kBalanceRequestHash,
        kNodeSigner,
        1700000001LL + 120, // 2 minutes skew
        60,
        nullptr,
        nullptr);

    TEST_ASSERT_EQUAL(VERIFY_TIMESTAMP_SKEW, st);
}

void test_extract_rpc_pair_result_fields() {
    // Shape returned inside verified uninode_getRpcPair result
    string result =
        "{\"chainId\":1,\"rpcs\":[\"https://ethereum-rpc.publicnode.com\","
        "\"https://eth.drpc.org\"],\"timestamp\":1700000099}";

    long long chain = 0;
    long long ts = 0;
    TEST_ASSERT_TRUE(extractTopLevelInt64(result, "chainId", chain));
    TEST_ASSERT_EQUAL(1, (int)chain);
    TEST_ASSERT_TRUE(extractTopLevelInt64(result, "timestamp", ts));
    TEST_ASSERT_EQUAL(1700000099, (int)ts);

    vector<string> rpcs;
    TEST_ASSERT_TRUE(extractTopLevelStringArray(result, "rpcs", rpcs));
    TEST_ASSERT_EQUAL(2, (int)rpcs.size());
    TEST_ASSERT_EQUAL_STRING("https://ethereum-rpc.publicnode.com", rpcs[0].c_str());
    TEST_ASSERT_EQUAL_STRING("https://eth.drpc.org", rpcs[1].c_str());

    // Reject non-https would be done by UniversalNode::getRpcPair; here just parsing
    string bad = "{\"chainId\":1,\"rpcs\":[1,2],\"timestamp\":1}";
    vector<string> badRpcs;
    TEST_ASSERT_FALSE(extractTopLevelStringArray(bad, "rpcs", badRpcs));
}

void test_rpc_pair_load_share_next_index() {
    // Per-request round-robin: after a call that started at i, next primary is i+1.
    TEST_ASSERT_EQUAL(0, rpcPairNextIndex(0, 1));
    TEST_ASSERT_EQUAL(0, rpcPairNextIndex(5, 1));
    TEST_ASSERT_EQUAL(1, rpcPairNextIndex(0, 2));
    TEST_ASSERT_EQUAL(0, rpcPairNextIndex(1, 2));
    // Negative start clamps to 0, then advances → 1
    TEST_ASSERT_EQUAL(1, rpcPairNextIndex(-1, 2));

    // Transport-failure retry uses the other node for the same request.
    TEST_ASSERT_EQUAL(1, rpcPairRetryIndex(0, 2));
    TEST_ASSERT_EQUAL(0, rpcPairRetryIndex(1, 2));
    TEST_ASSERT_EQUAL(0, rpcPairRetryIndex(0, 1));

    // Sequence of successful calls: 0 → 1 → 0 → 1 …
    int idx = 0;
    idx = rpcPairNextIndex(idx, 2);
    TEST_ASSERT_EQUAL(1, idx);
    idx = rpcPairNextIndex(idx, 2);
    TEST_ASSERT_EQUAL(0, idx);
    idx = rpcPairNextIndex(idx, 2);
    TEST_ASSERT_EQUAL(1, idx);

    // Failure on start 0 retries 1, then next primary still advances from start 0 → 1
    // (same as success path — load-share continues regardless of retry outcome).
    const int start = 0;
    const int retry = rpcPairRetryIndex(start, 2);
    TEST_ASSERT_EQUAL(1, retry);
    TEST_ASSERT_EQUAL(1, rpcPairNextIndex(start, 2));
}

void test_params_get_rpc_pair_feedback_json() {
    // No feedback → [chainId]
    TEST_ASSERT_EQUAL_STRING("[1]", paramsGetRpcPair(1, nullptr, 0, nullptr, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("[8453]", paramsGetRpcPair(8453, nullptr, 0, nullptr, 0).c_str());

    const char* workingOnly[] = {
        "https://ethereum-rpc.publicnode.com",
        "https://eth.drpc.org"
    };
    string onlyWorking = paramsGetRpcPair(1, nullptr, 0, workingOnly, 2);
    TEST_ASSERT_EQUAL_STRING(
        "[1,{\"working\":[\"https://ethereum-rpc.publicnode.com\",\"https://eth.drpc.org\"]}]",
        onlyWorking.c_str());

    const char* blockedOnly[] = { "https://eth.drpc.org" };
    string onlyBlocked = paramsGetRpcPair(1, blockedOnly, 1, nullptr, 0);
    TEST_ASSERT_EQUAL_STRING(
        "[1,{\"blocked\":[\"https://eth.drpc.org\"]}]",
        onlyBlocked.c_str());

    // Keys sorted: blocked before working
    const char* blocked[] = { "https://eth.drpc.org" };
    const char* working[] = { "https://ethereum-rpc.publicnode.com" };
    string both = paramsGetRpcPair(1, blocked, 1, working, 1);
    TEST_ASSERT_EQUAL_STRING(
        "[1,{\"blocked\":[\"https://eth.drpc.org\"],"
        "\"working\":[\"https://ethereum-rpc.publicnode.com\"]}]",
        both.c_str());

    // Cap at 8 URLs per list; null/empty entries skipped
    const char* many[10];
    char bufs[10][64];
    for (int i = 0; i < 10; i++) {
        snprintf(bufs[i], sizeof(bufs[i]), "https://n%d.example", i);
        many[i] = bufs[i];
    }
    string capped = paramsGetRpcPair(1, many, 10, nullptr, 0);
    TEST_ASSERT_TRUE(capped.find("https://n7.example") != string::npos);
    TEST_ASSERT_TRUE(capped.find("https://n8.example") == string::npos);

    const char* withEmpty[] = { "https://ok.example", "", nullptr, "https://two.example" };
    string skipped = paramsGetRpcPair(1, nullptr, 0, withEmpty, 4);
    TEST_ASSERT_EQUAL_STRING(
        "[1,{\"working\":[\"https://ok.example\",\"https://two.example\"]}]",
        skipped.c_str());
}

void test_rpc_pair_blocked_detection_and_rate_limit() {
    TEST_ASSERT_FALSE(rpcPairShouldMarkBlocked(0, 5));
    TEST_ASSERT_FALSE(rpcPairShouldMarkBlocked(2, 5)); // default threshold 3
    TEST_ASSERT_FALSE(rpcPairShouldMarkBlocked(3, 0)); // peer not succeeding
    TEST_ASSERT_TRUE(rpcPairShouldMarkBlocked(3, 1));
    TEST_ASSERT_TRUE(rpcPairShouldMarkBlocked(5, 2));
    TEST_ASSERT_TRUE(rpcPairShouldMarkBlocked(2, 1, 2)); // custom threshold

    TEST_ASSERT_TRUE(rpcPairCanRequestReplacement(1000, 0, false));
    TEST_ASSERT_TRUE(rpcPairCanRequestReplacement(1000, 500, false)); // never requested
    TEST_ASSERT_FALSE(rpcPairCanRequestReplacement(1000, 900, true, 500));
    TEST_ASSERT_TRUE(rpcPairCanRequestReplacement(1400, 900, true, 500));
    // Rollover-safe: now wrapped past last (0xFFFFFFF0 → 300 is 316 ms)
    TEST_ASSERT_TRUE(rpcPairCanRequestReplacement(300, 0xFFFFFFF0u, true, 200));
    TEST_ASSERT_FALSE(rpcPairCanRequestReplacement(100, 0xFFFFFFF0u, true, 200));

    TEST_ASSERT_TRUE(rpcPairLooksLikeTlsIntercept(-29184));
    TEST_ASSERT_TRUE(rpcPairLooksLikeTlsIntercept(-80));
    TEST_ASSERT_FALSE(rpcPairLooksLikeTlsIntercept(-1));
    TEST_ASSERT_FALSE(rpcPairLooksLikeTlsIntercept(0));

    TEST_ASSERT_TRUE(rpcPairLooksLikePlaintextHttp("HTTP/1.1 200 OK", 15));
    TEST_ASSERT_TRUE(rpcPairLooksLikePlaintextHttp("GET / HTTP/1.1", 14));
    TEST_ASSERT_TRUE(rpcPairLooksLikePlaintextHttp("POST /", 6));
    TEST_ASSERT_FALSE(rpcPairLooksLikePlaintextHttp("\x16\x03\x01", 3));
    TEST_ASSERT_FALSE(rpcPairLooksLikePlaintextHttp(nullptr, 10));
}

void test_rpc_pair_result_canonical_json_sorted_keys() {
    // Server canonicalJson sorts object keys (UTF-8): chainId, rpcs, timestamp.
    // Insertion order on the server object is chainId,timestamp,rpcs — hashing
    // must use the sorted form so result:<keccak256(canonicalJson(result))> matches.
    string unsorted =
        "{\"timestamp\":1700000001,\"rpcs\":[\"https://base-rpc.publicnode.com\","
        "\"https://base.drpc.org\"],\"chainId\":8453}";
    string expected =
        "{\"chainId\":8453,\"rpcs\":[\"https://base-rpc.publicnode.com\","
        "\"https://base.drpc.org\"],\"timestamp\":1700000001}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), canonicalJson(unsorted).c_str());

    // Response signing message uses blockNumber decimal "0" for getRpcPair
    string msg = buildResponseSigningMessage(
        "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266",
        "0x0123456789abcdef0123456789abcdef",
        8453,
        "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "0",
        1700000001LL,
        expected);
    TEST_ASSERT_TRUE(msg.find("blockNumber:0\n") != string::npos);
    TEST_ASSERT_TRUE(msg.find("result:0x") != string::npos);
    // Freshness is result.timestamp / cache age — blockNumber 0 is not a tip pin
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_canonical_sorts_object_keys);
    RUN_TEST(test_canonical_array_and_literals);
    RUN_TEST(test_canonical_nested);
    RUN_TEST(test_canonical_empty_array);
    RUN_TEST(test_params_eth_call_key_order);
    RUN_TEST(test_hash_params_matches_vector);
    RUN_TEST(test_build_request_signing_message);
    RUN_TEST(test_compute_request_hash);
    RUN_TEST(test_recover_device_from_request_signature);
    RUN_TEST(test_balance_params_and_request_hash);
    RUN_TEST(test_hash_result_and_response_message);
    RUN_TEST(test_recover_node_from_response_signature);
    RUN_TEST(test_verify_response_accepts_valid);
    RUN_TEST(test_verify_response_rejects_wrong_signer);
    RUN_TEST(test_verify_response_rejects_bad_request_hash);
    RUN_TEST(test_verify_response_rejects_timestamp_skew);
    RUN_TEST(test_extract_rpc_pair_result_fields);
    RUN_TEST(test_rpc_pair_load_share_next_index);
    RUN_TEST(test_params_get_rpc_pair_feedback_json);
    RUN_TEST(test_rpc_pair_blocked_detection_and_rate_limit);
    RUN_TEST(test_rpc_pair_result_canonical_json_sorted_keys);
    return UNITY_END();
}
