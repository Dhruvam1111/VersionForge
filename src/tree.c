#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h> // For directory scanning
#include <errno.h>

#include "tree.h"
#include "database.h"
#include "utils.h"

// A single entry (file or subdir) in a tree
struct tree_entry {
    char mode[7]; // "100644" for file, "040000" for dir
    char *name;
    unsigned char sha1[SHA_DIGEST_LENGTH];
};

// A dynamic list of tree entries
struct tree_entry_list {
    struct tree_entry **entries;
    int count;
    int capacity;
};

// Comparison function for qsort to sort entries by name
int compare_entries(const void *a, const void *b) {
    struct tree_entry *entry_a = *(struct tree_entry **)a;
    struct tree_entry *entry_b = *(struct tree_entry **)b;
    return strcmp(entry_a->name, entry_b->name);
}

// Write the serialized tree content and save as an object
int write_tree_object(struct tree_entry_list *list, char *out_hex, unsigned char *out_bin) {
    // Sort entries alphabetically
    qsort(list->entries, list->count, sizeof(struct tree_entry*), compare_entries);

    // Calculate total size
    size_t total_size = 0;
    for (int i = 0; i < list->count; i++) {
        // "mode<space>name<\0>sha1"
        total_size += strlen(list->entries[i]->mode) + 1 + strlen(list->entries[i]->name) + 1 + SHA_DIGEST_LENGTH;
    }

    char *buffer = malloc(total_size);
    char *ptr = buffer;
    for (int i = 0; i < list->count; i++) {
        struct tree_entry *e = list->entries[i];
        // Copy "mode "
        int mode_len = strlen(e->mode);
        memcpy(ptr, e->mode, mode_len);
        ptr[mode_len] = ' ';
        ptr += mode_len + 1;
        // Copy "name\0"
        int name_len = strlen(e->name);
        memcpy(ptr, e->name, name_len);
        ptr[name_len] = '\0';
        ptr += name_len + 1;
        // Copy binary SHA1
        memcpy(ptr, e->sha1, SHA_DIGEST_LENGTH);
        ptr += SHA_DIGEST_LENGTH;
    }

    // Write the actual tree object
    int result = write_object(buffer, total_size, "tree", out_hex, out_bin);
    
    free(buffer);
    return result;
}

int write_tree_recursive(const char *path, char *out_sha1_hex, unsigned char *out_sha1_binary) {
    DIR *d = opendir(path);
    if (!d) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    struct tree_entry_list list = { .entries = NULL, .count = 0, .capacity = 0 };

    while ((entry = readdir(d)) != NULL) {
        // Ignore ., .., and .minivcs
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".minivcs") == 0) {
            continue;
        }

        // We need the full path to stat
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat s;
        if (stat(full_path, &s) != 0) {
            perror("stat");
            continue;
        }

        // Add to our list
        if (list.count >= list.capacity) {
            list.capacity = (list.capacity == 0) ? 8 : list.capacity * 2;
            list.entries = realloc(list.entries, list.capacity * sizeof(struct tree_entry*));
        }
        
        struct tree_entry *te = malloc(sizeof(struct tree_entry));
        te->name = strdup(entry->d_name);

        if (S_ISDIR(s.st_mode)) {
            // It's a directory, recurse
            strcpy(te->mode, "040000");
            char sub_hex[41];
            if (write_tree_recursive(full_path, sub_hex, te->sha1) != 0) {
                fprintf(stderr, "Error: Failed to write tree for %s\n", full_path);
                // ... free memory ...
                closedir(d);
                return -1;
            }
        } else if (S_ISREG(s.st_mode)) {
            // It's a file, create a blob
            strcpy(te->mode, "100644"); // Note: not handling executable bit
            size_t file_size;
            char *content = read_file_to_buffer(full_path, &file_size);
            if (!content) {
                fprintf(stderr, "Error: Failed to read file %s\n", full_path);
                // ... free memory ...
                closedir(d);
                return -1;
            }
            char blob_hex[41];
            write_object(content, file_size, "blob", blob_hex, te->sha1);
            free(content);
        } else {
            // Not a file or dir, skip
            free(te->name);
            free(te);
            continue;
        }
        list.entries[list.count++] = te;
    }
    closedir(d);

    // Write the tree object for this directory
    int result = write_tree_object(&list, out_sha1_hex, out_sha1_binary);

    // Clean up
    for (int i = 0; i < list.count; i++) {
        free(list.entries[i]->name);
        free(list.entries[i]);
    }
    free(list.entries);

    return result;
}
