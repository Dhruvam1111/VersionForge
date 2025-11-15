#ifndef UTILS_H
#define UTILS_H

#include <stddef.h> // For size_t

/**
 * @brief Reads the entire content of a file into a dynamically allocated buffer.
 *
 * @param filepath Path to the file.
 * @param out_size Pointer to store the size of the file.
 * @return A char* buffer with the file content. The caller MUST free this.
 */
char* read_file_to_buffer(const char *filepath, size_t *out_size);

/**
 * @brief Resolves a ref (like "HEAD") to its full ref path (e.g., "refs/heads/main").
 *
 * @param ref_name The ref to resolve (e.g., "HEAD").
 * @param out_ref_path Buffer (at least 256 chars) to store the resolved path.
 * @return 0 on success, -1 on failure.
 */
int resolve_ref(const char *ref_name, char *out_ref_path);

/**
 * @brief Reads the SHA-1 hash from a given ref path.
 *
 * @param ref_path The full ref path (e.g., ".minivcs/refs/heads/main").
 * @param out_sha1_hex Buffer (at least 41 chars) to store the SHA-1.
 * @return 0 on success, -1 if the ref doesn't exist or can't be read.
 */
int read_ref(const char *ref_path, char *out_sha1_hex);

/**
 * @brief Updates a ref path to point to a new SHA-1 hash.
 *
 * @param ref_path The full ref path (e.g., ".minivcs/refs/heads/main").
 * @param sha1_hex The new 40-character SHA-1 hash.
 * @return 0 on success, -1 on failure.
 */
int update_ref(const char *ref_path, const char *sha1_hex);

#endif // UTILS_H
