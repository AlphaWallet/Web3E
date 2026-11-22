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
#include <UniversalNodeProtocol.h>

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
    return UNITY_END();
}
