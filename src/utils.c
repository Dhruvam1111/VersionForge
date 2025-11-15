#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

char* read_file_to_buffer(const char *filepath, size_t *out_size) {
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) {
        // perror("Error opening file"); // Keep this commented out
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize < 0) {
        fclose(f);
        return NULL;
    }
    char *buffer = malloc(fsize + 1);
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
    buffer[fsize] = '\0';
    *out_size = fsize;
    return buffer;
}

int resolve_ref(const char *ref_name, char *out_ref_path) {
    // ... (This function is unchanged) ...
    char head_path[256];
    snprintf(head_path, sizeof(head_path), ".minivcs/%s", ref_name);
    size_t file_size;
    char *content = read_file_to_buffer(head_path, &file_size);
    if (content == NULL) {
        return -1; 
    }
    if (strncmp(content, "ref: ", 5) == 0) {
        char *ref_path = content + 5;
        char *newline = strchr(ref_path, '\n');
        if (newline) {
            *newline = '\0';
        }
        snprintf(out_ref_path, 256, "%s", ref_path);
    } else {
        // Handle detached HEAD
        snprintf(out_ref_path, 256, "%s", ref_name);
    }
    free(content);
    return 0;
}

int read_ref(const char *ref_path, char *out_sha1_hex) {
    // ... (This function is unchanged, but now also handles detached HEADs) ...
    char full_path[256];
    snprintf(full_path, sizeof(full_path), ".minivcs/%s", ref_path);

    size_t file_size;
    char *content = read_file_to_buffer(full_path, &file_size);
    if (content == NULL) {
        return -1; 
    }
    
    if (strncmp(content, "ref: ", 5) == 0) {
        // This is a ref-to-ref (like HEAD -> refs/heads/main), so read again
        char *new_ref = content + 5;
        char *newline = strchr(new_ref, '\n');
        if (newline) *newline = '\0';
        free(content);
        return read_ref(new_ref, out_sha1_hex);
    }
    
    // This is a direct hash (or a branch file)
    strncpy(out_sha1_hex, content, 40);
    out_sha1_hex[40] = '\0';
    free(content);
    return 0;
}

int update_ref(const char *ref_path, const char *sha1_hex) {
    // ... (This function is unchanged) ...
    char full_path[256];
    snprintf(full_path, sizeof(full_path), ".minivcs/%s", ref_path);
    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        perror("Error opening ref file for writing");
        return -1;
    }
    fprintf(f, "%s\n", sha1_hex);
    fclose(f);
    return 0;
}

// *** NEW FUNCTION ***
void sha1_bin_to_hex(const unsigned char *sha1, char *hex_out) {
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(hex_out + (i * 2), "%02x", sha1[i]);
    }
    hex_out[40] = '\0';
}
