#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 

#include "status.h"
#include "utils.h"
#include "database.h"
#include "tree.h" 
#include "threadpool.h" 

void print_current_branch() {
    FILE *f = fopen(".minivcs/HEAD", "r");
    if (!f) { printf("On branch main\n"); return; }
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), f)) {
        if (strncmp(buffer, "ref: refs/heads/", 16) == 0) {
            char *branch_name = buffer + 16;
            branch_name[strcspn(buffer, "\n")] = 0; 
            printf("On branch %s\n", branch_name);
        } else {
            printf("HEAD detached at %.7s...\n", buffer);
        }
    }
    fclose(f);
}

static int get_tree_hash_from_commit(const char *commit_hash, char *out_tree_hash) {
    char *type, *data;
    size_t size;
    if (read_object(commit_hash, &type, &data, &size) != 0) return -1;
    char *tree_line = strstr(data, "tree ");
    if (!tree_line) { free(type); free(data); return -1; }
    strncpy(out_tree_hash, tree_line + 5, 40);
    out_tree_hash[40] = '\0';
    free(type); free(data);
    return 0;
}

int do_status() {
    print_current_branch();

    char head_commit_hash[41] = {0};
    char head_tree_hash[41] = {0};
    char current_tree_hash[41] = {0}; // Live Disk Hash
    
    int has_head = 0;
    char ref_path[256];
    if (resolve_ref("HEAD", ref_path) == 0 && read_ref(ref_path, head_commit_hash) == 0) {
        has_head = 1;
        get_tree_hash_from_commit(head_commit_hash, head_tree_hash);
    } else {
        printf("No commits yet\n");
    }

    // Initialize Pool
    threadpool_t *pool = threadpool_create(8, 256);

    // Calculate Live Disk Tree Hash
    if (write_tree_recursive(pool, ".", current_tree_hash, NULL) != 0) {
        if (!has_head) {
             printf("\nnothing to commit, working tree clean\n");
             return 0;
        }
    }
    threadpool_destroy(pool);

    // --- COMPARISON LOGIC ---
    // Compare Live Disk vs HEAD Commit
    
    if (has_head && strcmp(head_tree_hash, current_tree_hash) == 0) {
        printf("\nnothing to commit, working tree clean\n");
    } else {
        // Universal "dirty" message
        printf("\nChanges not committed:\n");
        printf("\tmodified/new: (use \"version_forge commit -m ...\" to record changes)\n");
    }

    return 0;
}
