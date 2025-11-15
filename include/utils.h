#ifndef UTILS_H
#define UTILS_H

#include <stddef.h> // For size_t
#include <openssl/sha.h> // For SHA_DIGEST_LENGTH

char* read_file_to_buffer(const char *filepath, size_t *out_size);

int resolve_ref(const char *ref_name, char *out_ref_path);
int read_ref(const char *ref_path, char *out_sha1_hex);
int update_ref(const char *ref_path, const char *sha1_hex);

/**
 * @brief Converts a 20-byte binary SHA-1 to a 40-char hex string.
 *
 * @param sha1 The 20-byte binary input.
 * @param hex_out A buffer (at least 41 chars) to store the hex string.
 */
void sha1_bin_to_hex(const unsigned char *sha1, char *hex_out);


#endif // UTILS_H
