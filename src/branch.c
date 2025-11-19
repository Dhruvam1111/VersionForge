#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "branch.h"
#include "utils.h"

int do_branch(const char *branch_name) {
    char current_head_hash[41];
    char ref_path[256];

    // 1. Get the current commit hash (HEAD)
    if (resolve_ref("HEAD", ref_path) != 0) {
        fprintf(stderr, "Error: Could not resolve HEAD. (Make a commit first?)\n");
        return 1;
    }
    if (read_ref(ref_path, current_head_hash) != 0) {
        fprintf(stderr, "Error: Could not read current commit hash.\n");
        return 1;
    }

    // 2. Create the new branch ref file
    // e.g., .minivcs/refs/heads/feature-login
    char new_branch_path[256];
    snprintf(new_branch_path, sizeof(new_branch_path), "refs/heads/%s", branch_name);

    if (update_ref(new_branch_path, current_head_hash) != 0) {
        fprintf(stderr, "Error: Could not create branch '%s'.\n", branch_name);
        return 1;
    }

    printf("Created branch '%s' at %s\n", branch_name, current_head_hash);
    return 0;
}
