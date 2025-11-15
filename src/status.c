#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "status.h"
#include "utils.h"
#include "database.h"
#include "tree.h" // We need this to build a new tree

// Helper to get the tree hash from a commit hash
static int get_tree_hash_from_commit(const char *commit_hash, char *out_tree_hash) {
    char *type = NULL;
    char *data = NULL;
    size_t size = 0;

    if (read_object(commit_hash, &type, &data, &size) != 0) return -1;
    if (strcmp(type, "commit") != 0) {
        free(type);
        free(data);
        return -1;
    }

    const char *tree_line = strstr(data, "tree ");
    if (tree_line == NULL) {
        free(type);
        free(data);
        return -1;
    }

    strncpy(out_tree_hash, tree_line + 5, 40);
    out_tree_hash[40] = '\0';
    
    free(type);
    free(data);
    return 0;
}

int do_status() {
    char head_commit_hash[41];
    char head_tree_hash[41];
    char current_tree_hash[41];
    char ref_path[256];

    // 1. Get HEAD commit
    if (resolve_ref("HEAD", ref_path) != 0 || read_ref(ref_path, head_commit_hash) != 0) {
        printf("On branch main\n");
        printf("No commits yet.\n");
        return 0;
    }
    
    // 2. Get tree hash from HEAD
    if (get_tree_hash_from_commit(head_commit_hash, head_tree_hash) != 0) {
        fprintf(stderr, "Error: Could not read HEAD commit tree.\n");
        return 1;
    }

    // 3. Build a tree from the current working directory
    // We pass NULL for the binary hash, we only want the hex hash
    if (write_tree_recursive(".", current_tree_hash, NULL) != 0) {
        fprintf(stderr, "Error: Could not scan working directory.\n");
        return 1;
    }

    // 4. Compare
    printf("On branch main\n"); // We've hardcoded this for now
    if (strcmp(head_tree_hash, current_tree_hash) == 0) {
        printf("working tree clean\n");
    } else {
        printf("Changes not staged for commit:\n");
        printf("  (use 'version_forge commit ...' to track changes)\n");
    }

    return 0;
}
