#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 

#include "commit.h"
#include "tree.h"
#include "utils.h"
#include "database.h"
#include "threadpool.h" 
#include "config.h" 

#define THREAD_COUNT 8
#define QUEUE_SIZE 256

int do_commit(const char *message) {
    char root_tree_hex[41];
    unsigned char root_tree_bin[SHA_DIGEST_LENGTH];

    // 1. Initialize Pool
    printf("Initializing thread pool with %d threads...\n", THREAD_COUNT);
    threadpool_t *pool = threadpool_create(THREAD_COUNT, QUEUE_SIZE);
    if (!pool) {
        fprintf(stderr, "Error creating thread pool.\n");
        return -1;
    }

    // 2. Build Tree from LIVE DISK
    if (write_tree_recursive(pool, ".", root_tree_hex, root_tree_bin) != 0) {
        printf("nothing to commit (no files found in repository).\n");
        threadpool_destroy(pool);
        return 0;
    }
    threadpool_destroy(pool);

    // 3. Check against HEAD (Idempotency Check)
    char current_ref_path[256];
    char head_commit_hash[41];
    
    if (resolve_ref("HEAD", current_ref_path) == 0 && read_ref(current_ref_path, head_commit_hash) == 0) {
        char *type, *data;
        size_t sz;
        if (read_object(head_commit_hash, &type, &data, &sz) == 0) {
             char *tree_ptr = strstr(data, "tree ");
             if (tree_ptr) {
                 char head_tree_hex[41];
                 strncpy(head_tree_hex, tree_ptr + 5, 40);
                 head_tree_hex[40] = '\0';
                 
                 // If live disk tree matches HEAD tree, abort.
                 if (strcmp(root_tree_hex, head_tree_hex) == 0) {
                     printf("On branch main\nnothing to commit, working tree clean\n");
                     free(type); free(data);
                     return 0;
                 }
             }
             free(type); free(data);
        }
    }

    printf("Root tree: %s\n", root_tree_hex);

    // 4. Finalize Commit (Author/Parent/Write/Update Ref)
    char parent_sha[41];
    int has_parent = 0;
    if (read_ref(current_ref_path, parent_sha) == 0) has_parent = 1;

    char author_name[128] = {0};
    char author_email[128] = {0};
    char author_str[256];

    if (get_config_value("user.name", author_name, 128) != 0) strcpy(author_name, "VersionForge User");
    if (get_config_value("user.email", author_email, 128) != 0) strcpy(author_email, "user@example.com");
    snprintf(author_str, 256, "%s <%s>", author_name, author_email);

    long timestamp = time(NULL);
    char *timezone = "+0000"; 
    size_t capacity = 1024 + strlen(message);
    char *content = malloc(capacity);
    int len = sprintf(content, "tree %s\n", root_tree_hex);
    if (has_parent) len += sprintf(content + len, "parent %s\n", parent_sha);
    
    // Fixed printf format to match types (long int for timestamp)
    len += sprintf(content + len, "author %s %ld %s\ncommitter %s %ld %s\n\n%s\n", 
                   author_str, timestamp, timezone, author_str, timestamp, timezone, message);
    
    char new_commit[41];
    write_object(content, len, "commit", new_commit, NULL);
    free(content);
    update_ref(current_ref_path, new_commit);

    printf("[%s] %s\n", current_ref_path, new_commit);
    return 0;
}
