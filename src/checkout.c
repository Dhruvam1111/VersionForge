#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "checkout.h"
#include "database.h"
#include "utils.h"

// Recursively delete a directory's contents, but NOT the dir itself
static void clean_directory(const char *path) {
    DIR *d = opendir(path);
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".minivcs") == 0 ||
	    strcmp(entry->d_name, "version_forge") == 0) { // IMPORTANT: Skip .minivcs and version_forge
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat s;
        if (lstat(full_path, &s) != 0) continue; // Use lstat for symlinks, etc.

        if (S_ISDIR(s.st_mode)) {
            clean_directory(full_path); // Recurse
            rmdir(full_path);
        } else {
            unlink(full_path); // Delete file
        }
    }
    closedir(d);
}

// Recursively restore a tree object to the given path
static int restore_tree(const char *tree_hash, const char *path) {
    char *type = NULL;
    char *data = NULL;
    size_t size = 0;

    if (read_object(tree_hash, &type, &data, &size) != 0) return -1;
    if (strcmp(type, "tree") != 0) {
        free(type); free(data);
        return -1;
    }

    char *ptr = data;
    while (ptr < data + size) {
        char *mode = ptr;
        char *name_start = strchr(ptr, ' ');
        if (!name_start) break;
        *name_start = '\0';
        ptr = name_start + 1;

        char *name = ptr;
        char *sha1_start = strchr(ptr, '\0');
        if (!sha1_start) break;
        ptr = sha1_start + 1 + SHA_DIGEST_LENGTH; // 1 for \0, 20 for hash
        
        unsigned char *sha1_bin = (unsigned char*)(sha1_start + 1);
        char sha1_hex[41];
        sha1_bin_to_hex(sha1_bin, sha1_hex);

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

        if (strcmp(mode, "040000") == 0) { // Directory
            mkdir(full_path, 0755);
            restore_tree(sha1_hex, full_path);
        } else { // File
            char *blob_type = NULL;
            char *blob_data = NULL;
            size_t blob_size = 0;
            if (read_object(sha1_hex, &blob_type, &blob_data, &blob_size) == 0) {
                FILE *f = fopen(full_path, "wb");
                if (f) {
                    fwrite(blob_data, 1, blob_size, f);
                    fclose(f);
                }
                free(blob_type);
                free(blob_data);
            }
        }
    }
    free(type);
    free(data);
    return 0;
}

int do_checkout(const char *target) {
    char commit_hash[41];
    char tree_hash[41];

    // For now, we only support checking out a specific commit hash
    // A full implementation would check for branch names
    if (strlen(target) != 40) {
        fprintf(stderr, "Error: Must provide a 40-char commit hash.\n");
        return 1;
    }
    strcpy(commit_hash, target);

    // 1. Get tree from commit
    char *type = NULL;
    char *data = NULL;
    size_t size = 0;
    if (read_object(commit_hash, &type, &data, &size) != 0) {
        fprintf(stderr, "Error: Could not read object %s\n", commit_hash);
        return 1;
    }
    if (strcmp(type, "commit") != 0) {
        fprintf(stderr, "Error: Object %s is not a commit.\n", commit_hash);
        free(type); free(data);
        return 1;
    }

    char *tree_line = strstr(data, "tree ");
    if (tree_line == NULL) {
        free(type); free(data);
        return -1;
    }
    strncpy(tree_hash, tree_line + 5, 40);
    tree_hash[40] = '\0';
    free(type);
    free(data);

    // 2. Clean working directory
    printf("Cleaning working directory...\n");
    clean_directory(".");

    // 3. Restore tree
    printf("Restoring commit %s...\n", commit_hash);
    if (restore_tree(tree_hash, ".") != 0) {
        fprintf(stderr, "Error restoring tree.\n");
        return 1;
    }

    // 4. Update HEAD to point to this commit (DETACHED HEAD)
    FILE *head = fopen(".minivcs/HEAD", "w");
    if (head) {
        fprintf(head, "%s\n", commit_hash);
        fclose(head);
    }
    printf("HEAD is now at %s\n", commit_hash);
    return 0;
}
