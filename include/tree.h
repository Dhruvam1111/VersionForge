#ifndef TREE_H
#define TREE_H

#include <openssl/sha.h>
#include "threadpool.h" 

/**
 * @brief Recursively writes a tree object.
 */
int write_tree_recursive(threadpool_t *pool, const char *path, char *out_sha1_hex, unsigned char *out_sha1_binary);

#endif // TREE_H
