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

/**
 * @brief Reads and decompresses an object from the store.
 *
 * @param hash The 40-char hex SHA-1 of the object.
 * @param out_type A pointer to store the object type (e.g., "blob"). MALLOC'D.
 * @param out_data A pointer to store the raw object data. MALLOC'D.
 * @param out_size A pointer to store the size of the raw data.
 * @return 0 on success, -1 on failure. Caller must free out_type and out_data.
 */
int read_object(const char *hash, char **out_type, char **out_data, size_t *out_size);


#endif // DATABASE_H
