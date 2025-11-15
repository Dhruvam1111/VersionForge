#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/sha.h> 
#include <zlib.h> // For compression AND decompression
#include "database.h"

static void sha1_to_hex(const unsigned char *sha1, char *hex_out) {
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(hex_out + (i * 2), "%02x", sha1[i]);
    }
    hex_out[40] = '\0';
}

int write_object(const void *data, size_t len, const char *type, 
                 char *out_sha1_hex, unsigned char *out_sha1_binary) {
    // ... (This function is unchanged from before) ...
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type, len) + 1;
    size_t full_size = header_len + len;
    void *full_content = malloc(full_size);
    if (!full_content) {
        perror("malloc");
        return -1;
    }
    memcpy(full_content, header, header_len);
    memcpy((char*)full_content + header_len, data, len);
    unsigned char sha1_binary_result[SHA_DIGEST_LENGTH];
    SHA1(full_content, full_size, sha1_binary_result);
    sha1_to_hex(sha1_binary_result, out_sha1_hex);
    if (out_sha1_binary != NULL) {
        memcpy(out_sha1_binary, sha1_binary_result, SHA_DIGEST_LENGTH);
    }
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    unsigned long compressed_size = deflateBound(&strm, full_size);
    void *compressed_data = malloc(compressed_size);
    if (!compressed_data) {
        perror("malloc");
        free(full_content);
        return -1;
    }
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
    if (deflate(&strm, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Error: deflate failed\n");
        deflateEnd(&strm);
        free(full_content);
        free(compressed_data);
        return -1;
    }
    compressed_size = strm.total_out;
    deflateEnd(&strm);
    free(full_content);
    char obj_dir[256];
    char obj_path[256];
    snprintf(obj_dir, sizeof(obj_dir), ".minivcs/objects/%.2s", out_sha1_hex);
    snprintf(obj_path, sizeof(obj_path), "%s/%.38s", obj_dir, out_sha1_hex + 2);
    if (mkdir(obj_dir, 0755) != 0 && errno != EEXIST) {
        perror("Error creating object directory");
        free(compressed_data);
        return -1;
    }
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

// *** NEW FUNCTION ***
int read_object(const char *hash, char **out_type, char **out_data, size_t *out_size) {
    char obj_path[256];
    snprintf(obj_path, sizeof(obj_path), ".minivcs/objects/%.2s/%.38s", hash, hash + 2);

    FILE *f = fopen(obj_path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t compressed_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *compressed_buffer = malloc(compressed_size);
    if (!compressed_buffer) {
        fclose(f);
        return -1;
    }
    if (fread(compressed_buffer, 1, compressed_size, f) != compressed_size) {
        fclose(f);
        free(compressed_buffer);
        return -1;
    }
    fclose(f);

    // Decompress (inflate)
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed_size;
    strm.next_in = compressed_buffer;

    if (inflateInit(&strm) != Z_OK) {
        free(compressed_buffer);
        return -1;
    }

    // We have to guess the output size.
    // Git's decompressed data is usually not more than 50x compression
    size_t decompressed_size_guess = compressed_size * 10;
    unsigned char *decompressed_buffer = malloc(decompressed_size_guess);

    strm.avail_out = decompressed_size_guess;
    strm.next_out = decompressed_buffer;

    int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    free(compressed_buffer);

    if (ret != Z_STREAM_END) {
        free(decompressed_buffer);
        return -1;
    }
    
    size_t actual_decompressed_size = strm.total_out;

    // Parse the header (e.g., "blob 123\0data...")
    char *header_end = (char*)memchr(decompressed_buffer, '\0', actual_decompressed_size);
    if (!header_end) {
        free(decompressed_buffer);
        return -1;
    }
    
    // Get type
    char *type_str = (char*)decompressed_buffer;
    char *size_str = (char*)memchr(type_str, ' ', header_end - type_str);
    if (!size_str) {
        free(decompressed_buffer);
        return -1;
    }
    
    *out_type = strndup(type_str, size_str - type_str);
    
    // Get data
    size_t header_len = (header_end - (char*)decompressed_buffer) + 1;
    *out_size = actual_decompressed_size - header_len;
    *out_data = malloc(*out_size);
    memcpy(*out_data, decompressed_buffer + header_len, *out_size);

    free(decompressed_buffer);
    return 0;
}
