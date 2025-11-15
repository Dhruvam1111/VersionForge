#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "commit.h"
#include "tree.h"
#include "utils.h"
#include "database.h"

int do_commit(const char *message) {
    char root_tree_hex[41];
    unsigned char root_tree_bin[SHA_DIGEST_LENGTH];

    // 1. Write the tree for the current directory
    printf("Building tree...\n");
    if (write_tree_recursive(".", root_tree_hex, root_tree_bin) != 0) {
        fprintf(stderr, "Error building tree.\n");
        return -1;
    }
    printf("Root tree: %s\n", root_tree_hex);

    // 2. Get the current ref path (e.g., "refs/heads/main")
    char current_ref_path[256];
    if (resolve_ref("HEAD", current_ref_path) != 0) {
        fprintf(stderr, "Error: Cannot resolve HEAD.\n");
        return -1;
    }

    // 3. Get the parent commit hash
    char parent_sha[41];
    int has_parent = 0;
    if (read_ref(current_ref_path, parent_sha) == 0) {
        has_parent = 1;
        printf("Parent: %s\n", parent_sha);
    } else {
        printf("No parent commit. Creating root commit.\n");
    }

    // 4. Format the commit object content
    // We'll hardcode the author for now
    char *author = "Dhruvam Panchal <dhruvam@example.com>";
    long timestamp = time(NULL);
    // TODO: Get timezone offset
    char *timezone = "+0530"; 

    // Use a dynamic buffer (string builder)
    size_t capacity = 1024;
    char *commit_content = malloc(capacity);
    int content_len = 0;
    
    // Add tree
    content_len += snprintf(commit_content + content_len, capacity - content_len,
                            "tree %s\n", root_tree_hex);
    // Add parent
    if (has_parent) {
        content_len += snprintf(commit_content + content_len, capacity - content_len,
                                "parent %s\n", parent_sha);
    }
    // Add author/committer
    content_len += snprintf(commit_content + content_len, capacity - content_len,
                            "author %s %ld %s\n", author, timestamp, timezone);
    content_len += snprintf(commit_content + content_len, capacity - content_len,
                            "committer %s %ld %s\n", author, timestamp, timezone);
    // Add message
    content_len += snprintf(commit_content + content_len, capacity - content_len,
                            "\n%s\n", message);

    // 5. Write the commit object
    char new_commit_hex[41];
    if (write_object(commit_content, content_len, "commit", new_commit_hex, NULL) != 0) {
        fprintf(stderr, "Error writing commit object.\n");
        free(commit_content);
        return -1;
    }
    free(commit_content);

    // 6. Update the ref (e.g., refs/heads/main) to point to the new commit
    if (update_ref(current_ref_path, new_commit_hex) != 0) {
        fprintf(stderr, "Error updating ref %s.\n", current_ref_path);
        return -1;
    }

    printf("Committed to %s: %s\n", current_ref_path, new_commit_hex);
    return 0;
}
