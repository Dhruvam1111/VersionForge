#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>

#include "network_utils.h"
#include "utils.h" // For read_file_to_buffer

void send_object_file(int sock, const char *filepath, const char *filename) {
    size_t size;
    char *content = read_file_to_buffer(filepath, &size);
    if (!content) return;

    char header[256];
    char buffer[1024];

    // 1. Send Header
    snprintf(header, sizeof(header), "OBJ %s %zu", filename, size);
    send(sock, header, strlen(header), 0);

    // 2. Wait for ACK
    read(sock, buffer, 1024); 

    // 3. Send Content
    send(sock, content, size, 0);
    free(content);

    // 4. Wait for Saved Confirmation
    read(sock, buffer, 1024); 
    printf("Transferred: %s\n", filename);
}

void scan_and_send(int sock, const char *base_path) {
    DIR *d = opendir(base_path);
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, entry->d_name);

        struct stat s;
        stat(full_path, &s);

        if (S_ISDIR(s.st_mode)) {
            scan_and_send(sock, full_path);
        } else {
            send_object_file(sock, full_path, entry->d_name);
        }
    }
    closedir(d);
}

void receive_object_file(int sock, char *hash, int size) {
    char dir[256];
    char path[256];
    
    snprintf(dir, sizeof(dir), ".minivcs/objects/%.2s", hash);
    // Ensure utils.h or sys/stat is included for mkdir
    #ifdef _WIN32
        mkdir(dir);
    #else
        mkdir(dir, 0755);
    #endif
    
    snprintf(path, sizeof(path), "%s/%.38s", dir, hash + 2);
    FILE *f = fopen(path, "wb");
    
    if (!f) {
        perror("File write failed");
        // Consume the bytes anyway to keep protocol in sync
        char *dump = malloc(size);
        read(sock, dump, size);
        free(dump);
        return;
    }

    char *file_buf = malloc(size);
    int remaining = size;
    int received = 0;
    while (remaining > 0) {
        int r = read(sock, file_buf + received, remaining);
        if (r <= 0) break;
        received += r;
        remaining -= r;
    }
    
    fwrite(file_buf, 1, received, f);
    fclose(f);
    free(file_buf);
    printf("Saved object: %s\n", hash);
}
