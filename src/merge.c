#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "merge.h"
#include "utils.h"
#include "database.h"
// We define a local helper instead of depending on checkout's internals
// to perform the "Overlay" logic without deleting existing files.

static int merge_tree_overlay(const char *tree_hash, const char *path) {
    char *type = NULL;
    char *data = NULL;
    size_t size = 0;

    if (read_object(tree_hash, &type, &data, &size) != 0) return -1;
    if (strcmp(type, "tree") != 0) { free(type); free(data); return -1; }

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
        ptr = sha1_start + 1 + SHA_DIGEST_LENGTH; 
        
        unsigned char *sha1_bin = (unsigned char*)(sha1_start + 1);
        char sha1_hex[41];
        sha1_bin_to_hex(sha1_bin, sha1_hex);

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

        // *** LOGIC CHANGE: WE DO NOT DELETE ANYTHING ***
        // We only write what is in the branch we are merging FROM.

        if (strcmp(mode, "040000") == 0) { 
            mkdir(full_path, 0755); // Ensure dir exists
            merge_tree_overlay(sha1_hex, full_path); // Recurse
        } else { 
            // File: Overwrite content if it exists, or create if new
            char *blob_type = NULL;
            char *blob_data = NULL;
            size_t blob_size = 0;
            if (read_object(sha1_hex, &blob_type, &blob_data, &blob_size) == 0) {
                FILE *f = fopen(full_path, "wb");
                if (f) {
                    fwrite(blob_data, 1, blob_size, f);
                    fclose(f);
                    // printf("Merged file: %s\n", name);
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

int do_merge(const char *branch_name) {
    char current_branch[256];
    char target_branch_ref[256];
    char current_hash[41];
    char target_hash[41];

    printf("Merging branch '%s' into current HEAD...\n", branch_name);

    // 1. Resolve Target
    snprintf(target_branch_ref, sizeof(target_branch_ref), "refs/heads/%s", branch_name);
    if (read_ref(target_branch_ref, target_hash) != 0) {
        fprintf(stderr, "Error: Branch '%s' does not exist.\n", branch_name);
        return 1;
    }

    // 2. Resolve HEAD
    char head_content[256];
    if (resolve_ref("HEAD", head_content) != 0) return 1;
    if (read_ref(head_content, current_hash) != 0) {
        strncpy(current_hash, head_content, 40);
        current_hash[40] = '\0';
    }

    printf("Current HEAD: %s\n", current_hash);
    printf("Merging Ref:  %s\n", target_hash);

    if (strcmp(current_hash, target_hash) == 0) {
        printf("Already up to date.\n");
        return 0;
    }

    // 3. Get Target Tree Hash
    // We need the tree hash of the branch we are merging IN
    char *type = NULL, *data = NULL;
    size_t size;
    char target_tree_hash[41];
    
    if (read_object(target_hash, &type, &data, &size) == 0) {
        char *tree_line = strstr(data, "tree ");
        if (tree_line) {
             strncpy(target_tree_hash, tree_line + 5, 40);
             target_tree_hash[40] = '\0';
        }
        free(type); free(data);
    } else {
        fprintf(stderr, "Error reading target commit.\n");
        return 1;
    }

    // 4. Perform Overlay Merge (Union)
    // This adds 'feature' files to 'main' files without deleting 'main' files.
    printf("Performing Union Merge (Appending changes)...\n");
    if (merge_tree_overlay(target_tree_hash, ".") != 0) {
        fprintf(stderr, "Merge failed.\n");
        return 1;
    }

    // 5. Update HEAD to point to the new state
    // Note: In a real merge, we would create a NEW merge commit with two parents.
    // For this simplified "Fast Forward/Union", we move the pointer to the target.
    if (strncmp(head_content, "refs/heads/", 11) == 0) {
        update_ref(head_content, target_hash);
        printf("Merge complete. HEAD updated to %s\n", target_hash);
    } else {
        printf("Merge complete (Detached HEAD).\n");
    }

    return 0;
}
