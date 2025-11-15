#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

char* read_file_to_buffer(const char *filepath, size_t *out_size) {
    FILE *f = fopen(filepath, "rb"); // Read in binary mode
    if (f == NULL) {
        return NULL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        fclose(f);
        return NULL;
    }

    char *buffer = malloc(fsize + 1); // +1 for null terminator
    if (buffer == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for file buffer\n");
        fclose(f);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, fsize, f);
    fclose(f);

    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Error reading file\n");
        free(buffer);
        return NULL;
    }

    buffer[fsize] = '\0'; // Null-terminate the buffer
    *out_size = fsize;
    return buffer;
}

int resolve_ref(const char *ref_name, char *out_ref_path) {
    char head_path[256];
    snprintf(head_path, sizeof(head_path), ".minivcs/%s", ref_name);

    size_t file_size;
    char *content = read_file_to_buffer(head_path, &file_size);
    if (content == NULL) {
        return -1; // Can't read HEAD
    }

    if (strncmp(content, "ref: ", 5) == 0) {
        // It's a symbolic ref, like "ref: refs/heads/main"
        // Skip "ref: " and copy the rest
        char *ref_path = content + 5;
        // Trim trailing newline
        char *newline = strchr(ref_path, '\n');
        if (newline) {
            *newline = '\0';
        }
        snprintf(out_ref_path, 256, "%s", ref_path);
    } else {
        // It's a detached HEAD, just copy the ref name
        snprintf(out_ref_path, 256, "%s", ref_name);
    }
    
    free(content);
    return 0;
}

int read_ref(const char *ref_path, char *out_sha1_hex) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), ".minivcs/%s", ref_path);

    size_t file_size;
    char *content = read_file_to_buffer(full_path, &file_size);
    if (content == NULL) {
        return -1; // Ref doesn't exist
    }

    // Copy hash, trim newline
    memcpy(out_sha1_hex, content, 40);
    out_sha1_hex[40] = '\0';
    free(content);
    return 0;
}

int update_ref(const char *ref_path, const char *sha1_hex) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), ".minivcs/%s", ref_path);

    // TODO: We should create directories if they don't exist
    
    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        perror("Error opening ref file for writing");
        return -1;
    }

    fprintf(f, "%s\n", sha1_hex);
    fclose(f);
    return 0;
}
