/**
 * HTTP Parser Unit Tests (Native)
 * 
 * Tests the HttpParser class that handles HTTP response parsing
 * including headers, Content-Length, and chunked transfer encoding.
 * 
 * Runs natively (no ESP32 required).
 */

#include <unity.h>
#include <string>
#include <cstring>
#include <HttpParser.h>

void test_parse_status_line_success() {
    HttpParser parser;
    
    TEST_ASSERT_TRUE(parser.parseStatusLine("HTTP/1.1 200 OK"));
    TEST_ASSERT_EQUAL(200, parser.statusCode);
    
    TEST_ASSERT_TRUE(parser.parseStatusLine("HTTP/1.0 404 Not Found"));
    TEST_ASSERT_EQUAL(404, parser.statusCode);
    
    TEST_ASSERT_TRUE(parser.parseStatusLine("HTTP/1.1 500 Internal Server Error"));
    TEST_ASSERT_EQUAL(500, parser.statusCode);
}

void test_parse_status_line_invalid() {
    HttpParser parser;
    
    TEST_ASSERT_FALSE(parser.parseStatusLine("Not HTTP"));
    TEST_ASSERT_FALSE(parser.parseStatusLine("HTTP/1.1"));
    TEST_ASSERT_FALSE(parser.parseStatusLine(""));
}

void test_parse_content_length() {
    HttpParser parser;
    
    parser.parseHeader("Content-Length: 1234");
    TEST_ASSERT_TRUE(parser.hasContentLength);
    TEST_ASSERT_EQUAL(1234, parser.contentLength);
    
    // Case insensitive
    parser.contentLength = 0;
    parser.hasContentLength = false;
    parser.parseHeader("content-length: 5678");
    TEST_ASSERT_TRUE(parser.hasContentLength);
    TEST_ASSERT_EQUAL(5678, parser.contentLength);
    
    parser.parseHeader("CONTENT-LENGTH: 9999");
    TEST_ASSERT_EQUAL(9999, parser.contentLength);
}

void test_parse_transfer_encoding_chunked() {
    HttpParser parser;
    
    parser.parseHeader("Transfer-Encoding: chunked");
    TEST_ASSERT_TRUE(parser.isChunked);
    
    // Case insensitive
    parser.isChunked = false;
    parser.parseHeader("transfer-encoding: chunked");
    TEST_ASSERT_TRUE(parser.isChunked);
    
    parser.isChunked = false;
    parser.parseHeader("TRANSFER-ENCODING: CHUNKED");
    TEST_ASSERT_TRUE(parser.isChunked);
    
    // With whitespace
    parser.isChunked = false;
    parser.parseHeader("Transfer-Encoding:  chunked");
    TEST_ASSERT_TRUE(parser.isChunked);
}

void test_decode_single_chunk() {
    HttpParser parser;
    char outputBuf[256];
    size_t consumed, written;
    
    // Simple chunk: "5\r\nHello\r\n"
    const char* chunk1 = "5\r\nHello\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(chunk1, strlen(chunk1), 
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    TEST_ASSERT_EQUAL(10, consumed); // "5\r\nHello\r\n"
    TEST_ASSERT_EQUAL(5, written);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello", outputBuf, 5);
}

void test_decode_last_chunk() {
    HttpParser parser;
    char outputBuf[256];
    size_t consumed, written;
    
    // Last chunk (size 0): "0\r\n\r\n"
    const char* lastChunk = "0\r\n\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(lastChunk, strlen(lastChunk),
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    TEST_ASSERT_EQUAL(5, consumed);
    TEST_ASSERT_EQUAL(0, written); // No data in last chunk
}

void test_decode_chunk_incomplete() {
    HttpParser parser;
    char outputBuf[256];
    size_t consumed, written;
    
    // Incomplete chunk (missing data): "5\r\nHe"
    const char* incomplete = "5\r\nHe";
    TEST_ASSERT_FALSE(parser.decodeChunk(incomplete, strlen(incomplete),
                                         outputBuf, sizeof(outputBuf),
                                         consumed, written));
    
    // Incomplete size line: "5"
    TEST_ASSERT_FALSE(parser.decodeChunk("5", 1, outputBuf, sizeof(outputBuf),
                                         consumed, written));
    
    // Incomplete size line: "5\r"
    TEST_ASSERT_FALSE(parser.decodeChunk("5\r", 2, outputBuf, sizeof(outputBuf),
                                         consumed, written));
}

void test_decode_chunk_hex_size() {
    HttpParser parser;
    char outputBuf[256];
    size_t consumed, written;
    
    // Hex size: "a\r\n0123456789\r\n" (10 bytes)
    const char* hexChunk = "a\r\n0123456789\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(hexChunk, strlen(hexChunk),
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    TEST_ASSERT_EQUAL(10, written);
    TEST_ASSERT_EQUAL_STRING_LEN("0123456789", outputBuf, 10);
    
    // Uppercase hex: "A\r\n0123456789\r\n"
    const char* hexChunkUpper = "A\r\n0123456789\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(hexChunkUpper, strlen(hexChunkUpper),
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    TEST_ASSERT_EQUAL(10, written);
}

void test_decode_multiple_chunks_sequential() {
    HttpParser parser;
    char outputBuf[256];
    char fullOutput[256];
    size_t totalWritten = 0;
    size_t consumed, written;
    
    // Simulate receiving chunks one at a time
    // Chunk 1: "5\r\nHello\r\n"
    const char* chunk1 = "5\r\nHello\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(chunk1, strlen(chunk1),
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    memcpy(fullOutput + totalWritten, outputBuf, written);
    totalWritten += written;
    
    // Chunk 2: "7\r\n, World\r\n"
    const char* chunk2 = "7\r\n, World\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(chunk2, strlen(chunk2),
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    memcpy(fullOutput + totalWritten, outputBuf, written);
    totalWritten += written;
    
    // Last chunk: "0\r\n\r\n"
    const char* chunk3 = "0\r\n\r\n";
    TEST_ASSERT_TRUE(parser.decodeChunk(chunk3, strlen(chunk3),
                                        outputBuf, sizeof(outputBuf),
                                        consumed, written));
    
    // Verify complete message
    TEST_ASSERT_EQUAL(12, totalWritten);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello, World", fullOutput, 12);
}

void test_parse_other_headers_ignored() {
    HttpParser parser;
    
    // These should be ignored (not crash)
    parser.parseHeader("Server: Apache/2.4");
    parser.parseHeader("Date: Mon, 15 Nov 2026 12:00:00 GMT");
    parser.parseHeader("Connection: close");
    
    // Should not have set content length or chunked
    TEST_ASSERT_FALSE(parser.hasContentLength);
    TEST_ASSERT_FALSE(parser.isChunked);
}

void test_real_world_response_simulation() {
    HttpParser parser;
    
    // Simulate real HTTP response parsing
    TEST_ASSERT_TRUE(parser.parseStatusLine("HTTP/1.1 200 OK"));
    TEST_ASSERT_EQUAL(200, parser.statusCode);
    
    parser.parseHeader("Date: Mon, 15 Nov 2026 12:00:00 GMT");
    parser.parseHeader("Server: nginx");
    parser.parseHeader("Content-Type: application/json");
    parser.parseHeader("Content-Length: 42");
    parser.parseHeader("Connection: close");
    
    TEST_ASSERT_TRUE(parser.hasContentLength);
    TEST_ASSERT_EQUAL(42, parser.contentLength);
    TEST_ASSERT_FALSE(parser.isChunked);
}

void test_chunked_response_simulation() {
    HttpParser parser;
    
    TEST_ASSERT_TRUE(parser.parseStatusLine("HTTP/1.1 200 OK"));
    parser.parseHeader("Content-Type: application/json");
    parser.parseHeader("Transfer-Encoding: chunked");
    parser.parseHeader("Connection: close");
    
    TEST_ASSERT_TRUE(parser.isChunked);
    TEST_ASSERT_FALSE(parser.hasContentLength);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_status_line_success);
    RUN_TEST(test_parse_status_line_invalid);
    RUN_TEST(test_parse_content_length);
    RUN_TEST(test_parse_transfer_encoding_chunked);
    RUN_TEST(test_decode_single_chunk);
    RUN_TEST(test_decode_last_chunk);
    RUN_TEST(test_decode_chunk_incomplete);
    RUN_TEST(test_decode_chunk_hex_size);
    RUN_TEST(test_decode_multiple_chunks_sequential);
    RUN_TEST(test_parse_other_headers_ignored);
    RUN_TEST(test_real_world_response_simulation);
    RUN_TEST(test_chunked_response_simulation);
    
    return UNITY_END();
}
