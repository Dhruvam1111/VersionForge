#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
 
#include <zlib.h> // For zlib compression
#include "database.h" // Already includes openssl/sha.h

// Helper function to convert binary SHA-1 to hex string
static void sha1_to_hex(const unsigned char *sha1, char *hex_out) {
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(hex_out + (i * 2), "%02x", sha1[i]);
    }
    hex_out[40] = '\0';
}

int write_object(const void *data, size_t len, const char *type, char *out_sha1_hex, unsigned char *out_sha1_binary) {
    // 1. Create the header: "type <length>\0"
    // We use 64 bytes as a safe buffer size for the header
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type, len) + 1; // +1 for null byte

    // 2. Create the full content to be hashed: header + data
    size_t full_size = header_len + len;
    void *full_content = malloc(full_size);
    if (!full_content) {
        perror("malloc");
        return -1;
    }
    memcpy(full_content, header, header_len);
    memcpy((char*)full_content + header_len, data, len);

    // 3. Calculate the SHA-1 hash 
    unsigned char sha1_binary_result[SHA_DIGEST_LENGTH]; // 20 bytes
    SHA1(full_content, full_size, sha1_binary_result);
    
    // Convert binary hash to hex string for the filename and output
    sha1_to_hex(sha1_binary_result, out_sha1_hex);

    // Copy binary hash to output buffer if provided
    if (out_sha1_binary != NULL) {
        memcpy(out_sha1_binary, sha1_binary_result, SHA_DIGEST_LENGTH);
    }

    // 4. zlib-compress the full content 
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // 'deflateBound' gives a worst-case size for the compressed data
    unsigned long compressed_size = deflateBound(&strm, full_size);
    void *compressed_data = malloc(compressed_size);
    if (!compressed_data) {
        perror("malloc");
        free(full_content);
        return -1;
    }

    // Initialize the compressor
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        fprintf(stderr, "Error: deflateInit failed\n");
        free(full_content);
        free(compressed_data);
        return -1;
    }

    strm.avail_in = full_size;
    strm.next_in = (Bytef*)full_content;
    strm.avail_out = compressed_size;
    strm.next_out = (Bytef*)compressed_data;

    // Compress in one go
    if (deflate(&strm, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Error: deflate failed\n");
        deflateEnd(&strm);
        free(full_content);
        free(compressed_data);
        return -1;
    }
    compressed_size = strm.total_out; // Get the actual compressed size
    deflateEnd(&strm);
    free(full_content); // We are done with the uncompressed data

    // 5. Write the compressed data to the object store 
    char obj_dir[256];
    char obj_path[256];
    
    // First 2 chars of hash are the directory
    snprintf(obj_dir, sizeof(obj_dir), ".minivcs/objects/%.2s", out_sha1_hex);
    // Remaining 38 chars are the filename
    snprintf(obj_path, sizeof(obj_path), "%s/%.38s", obj_dir, out_sha1_hex + 2);

    // Create the directory (e.g., .minivcs/objects/83)
    if (mkdir(obj_dir, 0755) != 0 && errno != EEXIST) {
        perror("Error creating object directory");
        free(compressed_data);
        return -1;
    }

    // Write the compressed file
    FILE *f = fopen(obj_path, "wb");
    if (!f) {
        perror("Error opening object file for writing");
        free(compressed_data);
        return -1;
    }

    fwrite(compressed_data, 1, compressed_size, f);
    fclose(f);
    free(compressed_data);

    return 0;
}
