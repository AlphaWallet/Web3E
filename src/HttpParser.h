#ifndef WEB3E_HTTPPARSER_H
#define WEB3E_HTTPPARSER_H

#include <string>
#include <cstring>
#include <cctype>

using namespace std;

/**
 * @brief Lightweight HTTP response parser for Web3E
 * 
 * Handles:
 * - HTTP status line parsing
 * - Header parsing (Content-Length, Transfer-Encoding: chunked)
 * - Chunked transfer encoding decoding
 * - Case-insensitive header matching
 */
class HttpParser {
public:
    int statusCode;
    size_t contentLength;
    bool isChunked;
    bool hasContentLength;
    
    HttpParser() : statusCode(0), contentLength(0), isChunked(false), hasContentLength(false) {}
    
    /**
     * @brief Parse HTTP status line
     * @param line Status line (e.g., "HTTP/1.1 200 OK")
     * @return true if successfully parsed
     */
    bool parseStatusLine(const char* line) {
        // Expected format: "HTTP/1.x 200 OK"
        if (strncmp(line, "HTTP/", 5) != 0) {
            return false;
        }
        
        // Find first space after HTTP/1.x
        const char* statusStart = strchr(line + 5, ' ');
        if (!statusStart) {
            return false;
        }
        
        statusCode = atoi(statusStart + 1);
        return statusCode > 0;
    }
    
    /**
     * @brief Parse HTTP header line
     * @param line Header line (e.g., "Content-Length: 1234")
     */
    void parseHeader(const char* line) {
        // Case-insensitive search for Content-Length
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            contentLength = atoi(line + 15);
            hasContentLength = true;
        }
        // Case-insensitive search for Transfer-Encoding: chunked
        else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) {
            const char* value = line + 18;
            // Skip whitespace
            while (*value && isspace(*value)) value++;
            if (strncasecmp(value, "chunked", 7) == 0) {
                isChunked = true;
            }
        }
    }
    
    /**
     * @brief Decode a single HTTP chunk
     * @param chunkData Pointer to chunk data (starts with hex size)
     * @param availableBytes Total bytes available in buffer
     * @param outputBuf Output buffer for decoded data
     * @param outputSize Size of output buffer
     * @param bytesConsumed Output: how many bytes were consumed from input
     * @param bytesWritten Output: how many bytes were written to output
     * @return true if chunk was successfully decoded, false if incomplete
     */
    static bool decodeChunk(const char* chunkData, size_t availableBytes,
                           char* outputBuf, size_t outputSize,
                           size_t& bytesConsumed, size_t& bytesWritten) {
        bytesConsumed = 0;
        bytesWritten = 0;
        
        if (availableBytes < 3) { // Need at least "0\r\n"
            return false;
        }
        
        // Parse chunk size (hex)
        size_t chunkSize = 0;
        size_t i = 0;
        while (i < availableBytes && chunkData[i] != '\r' && chunkData[i] != '\n') {
            char c = chunkData[i];
            if (c >= '0' && c <= '9') {
                chunkSize = (chunkSize << 4) | (c - '0');
            } else if (c >= 'a' && c <= 'f') {
                chunkSize = (chunkSize << 4) | (c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                chunkSize = (chunkSize << 4) | (c - 'A' + 10);
            } else {
                break; // End of size or chunk extension
            }
            i++;
        }
        
        // Skip to \r\n after chunk size
        while (i < availableBytes && chunkData[i] != '\n') {
            i++;
        }
        if (i >= availableBytes) {
            return false; // Incomplete
        }
        i++; // Skip \n
        
        // Check if we have the full chunk data + trailing \r\n
        if (i + chunkSize + 2 > availableBytes) {
            return false; // Incomplete
        }
        
        // Copy chunk data to output
        if (chunkSize > 0 && bytesWritten + chunkSize <= outputSize) {
            memcpy(outputBuf + bytesWritten, chunkData + i, chunkSize);
            bytesWritten = chunkSize;
        }
        
        bytesConsumed = i + chunkSize + 2; // Size line + data + \r\n
        return true;
    }
};

#endif
