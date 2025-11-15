#ifndef TREE_H
#define TREE_H

#include <openssl/sha.h> // For SHA_DIGEST_LENGTH

/**
 * @brief Recursively writes a tree object for the given directory path.
 *
 * Scans the directory, creates blobs for files, and recursively
 * creates trees for subdirectories.
 *
 * @param path The path to the directory to build a tree for (e.g., ".").
 * @param out_sha1_hex Buffer (at least 41 chars) to store the tree's hex SHA-1.
 * @param out_sha1_binary Buffer (at least SHA_DIGEST_LENGTH chars) to store
 * the tree's binary SHA-1.
 * @return 0 on success, -1 on failure.
 */
int write_tree_recursive(const char *path, char *out_sha1_hex, unsigned char *out_sha1_binary);

#endif // TREE_H
