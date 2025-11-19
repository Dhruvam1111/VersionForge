#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

/**
 * @brief Sends a specific object file over the socket.
 * Follows protocol: Header -> Wait ACK -> Content -> Wait Saved.
 */
void send_object_file(int sock, const char *filepath, const char *filename);

/**
 * @brief Recursively scans a directory and sends all object files found.
 */
void scan_and_send(int sock, const char *base_path);

/**
 * @brief Receives a binary object stream and saves it to disk.
 */
void receive_object_file(int sock, char *hash, int size);

#endif // NETWORK_UTILS_H
