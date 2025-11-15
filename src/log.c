#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "utils.h"
#include "database.h"

// Helper to parse commit data
// Finds the first "parent <hash>" line and returns the hash
static int get_parent_hash(const char *commit_data, char *out_parent_hash) {
    const char *parent_line = strstr(commit_data, "parent ");
    if (parent_line == NULL) {
        return -1; // No parent
    }

    // "parent " is 7 chars. Copy the 40-char hash after it.
    strncpy(out_parent_hash, parent_line + 7, 40);
    out_parent_hash[40] = '\0';
    return 0;
}

int do_log() {
    char current_hash[41];
    char ref_path[256];

    // 1. Start from HEAD
    if (resolve_ref("HEAD", ref_path) != 0) {
        fprintf(stderr, "Error: Could not resolve HEAD.\n");
        return 1;
    }
    if (read_ref(ref_path, current_hash) != 0) {
        fprintf(stderr, "No commits yet.\n");
        return 0;
    }

    // 2. Loop by following parent hashes
    while (1) {
        char *type = NULL;
        char *commit_data = NULL;
        size_t commit_size = 0;

        // 3. Read the commit object
        if (read_object(current_hash, &type, &commit_data, &commit_size) != 0) {
            fprintf(stderr, "Error: Could not read commit %s\n", current_hash);
            break;
        }

        if (type == NULL || strcmp(type, "commit") != 0) {
            fprintf(stderr, "Error: Object %s is not a commit.\n", current_hash);
            free(type);
            free(commit_data);
            break;
        }

        // 4. Print commit info
        printf("commit %s\n", current_hash);

        // Simple parser: find author and message
        char *author_line = strstr(commit_data, "author ");
        char *message_start = strstr(commit_data, "\n\n");

        if (author_line) {
            char *end_of_line = strstr(author_line, "\n");
            if (end_of_line) {
                printf("%.*s\n", (int)(end_of_line - author_line), author_line);
            }
        }
        if (message_start) {
            printf("\n%s\n", message_start + 2); // +2 to skip the \n\n
        }

        // 5. Find the parent and continue
        char parent_hash[41];
        if (get_parent_hash(commit_data, parent_hash) == 0) {
            strncpy(current_hash, parent_hash, 41);
        } else {
            break; // No more parents
        }

        free(type);
        free(commit_data);
    }

    return 0;
}
