#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h> // For size_t
#include <openssl/sha.h> // For SHA_DIGEST_LENGTH

/**
 * @brief Creates a version forge object and saves it to the object store.
 *
 * @param data The raw data to store.
 * @param len The length of the data.
 * @param type The type of object ("blob", "tree", "commit").
 * @param out_sha1_hex A buffer (at least 41 chars) to store the resulting hex SHA-1.
 * @param out_sha1_binary A buffer (at least SHA_DIGEST_LENGTH chars) to 
 * store the resulting binary SHA-1. Can be NULL.
 * @return 0 on success, -1 on failure.
 */
int write_object(const void *data, size_t len, const char *type, 
                 char *out_sha1_hex, unsigned char *out_sha1_binary);

#endif // DATABASE_H
