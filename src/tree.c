#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h> 

#include "tree.h"
#include "database.h"
#include "utils.h"
#include "vf_signals.h" 

// --- Synchronization Structures ---
struct tree_entry {
    char mode[7];
    char *name;
    unsigned char sha1[SHA_DIGEST_LENGTH];
};

struct dir_context {
    struct tree_entry **entries; 
    int capacity;
    int count;
    int tasks_remaining;
    pthread_mutex_t lock;
    pthread_cond_t done;
    int error_occurred; // Correct member name
};

struct worker_args {
    char *filepath;
    struct tree_entry *entry; 
    struct dir_context *ctx;  
};

// --- Worker Function ---
void process_file_task(void *arg) {
    struct worker_args *args = (struct worker_args *)arg;
    
    if (shutdown_requested) {
        pthread_mutex_lock(&args->ctx->lock); args->ctx->error_occurred = 1; args->ctx->tasks_remaining--;
        if (args->ctx->tasks_remaining == 0) pthread_cond_signal(&args->ctx->done);
        pthread_mutex_unlock(&args->ctx->lock); free(args->filepath); free(args);
        return;
    }

    size_t file_size;
    char *content = read_file_to_buffer(args->filepath, &file_size);
    int result = -1;

    if (content) {
        char blob_hex[41];
        result = write_object(content, file_size, "blob", blob_hex, args->entry->sha1);
        free(content);
    }

    pthread_mutex_lock(&args->ctx->lock);
    if (result != 0) {
        args->ctx->error_occurred = 1; // Corrected usage
        fprintf(stderr, "Error hashing file: %s\n", args->filepath);
    }
    args->ctx->tasks_remaining--;
    if (args->ctx->tasks_remaining == 0) pthread_cond_signal(&args->ctx->done);
    pthread_mutex_unlock(&args->ctx->lock);

    free(args->filepath); free(args);
}

int compare_entries(const void *a, const void *b) {
    struct tree_entry *entry_a = *(struct tree_entry **)a;
    struct tree_entry *entry_b = *(struct tree_entry **)b;
    return strcmp(entry_a->name, entry_b->name);
}

// --- Main Recursive Function (Stable Signature) ---
int write_tree_recursive(threadpool_t *pool, const char *path, char *out_sha1_hex, unsigned char *out_sha1_binary) {
    DIR *d = opendir(path);
    if (!d) return -1;

    struct dirent *dir_entry;
    struct dir_context ctx;
    ctx.capacity = 10;
    ctx.count = 0;
    ctx.tasks_remaining = 0;
    ctx.error_occurred = 0;
    ctx.entries = malloc(sizeof(struct tree_entry*) * ctx.capacity);
    pthread_mutex_init(&ctx.lock, NULL);
    pthread_cond_init(&ctx.done, NULL);

    while ((dir_entry = readdir(d)) != NULL) {
        if (shutdown_requested) { ctx.error_occurred = 1; break; }

        if (strcmp(dir_entry->d_name, ".") == 0 || 
            strcmp(dir_entry->d_name, "..") == 0 || 
            strcmp(dir_entry->d_name, ".minivcs") == 0 ||
            strcmp(dir_entry->d_name, "version_forge") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, dir_entry->d_name);

        struct stat s;
        if (stat(full_path, &s) != 0) continue;

        if (ctx.count >= ctx.capacity) {
            ctx.capacity *= 2;
            ctx.entries = realloc(ctx.entries, sizeof(struct tree_entry*) * ctx.capacity);
        }

        struct tree_entry *te = malloc(sizeof(struct tree_entry));
        te->name = strdup(dir_entry->d_name);
        int should_add = 0;

        if (S_ISDIR(s.st_mode)) {
            strcpy(te->mode, "040000");
            char sub_hex[41];
            if (write_tree_recursive(pool, full_path, sub_hex, te->sha1) == 0) {
                should_add = 1;
            } else {
                free(te->name); free(te);
            }
        } else {
            // File: Always hash the live content from disk (Commit All Logic)
            strcpy(te->mode, "100644");
            
            if (pool) {
                struct worker_args *args = malloc(sizeof(struct worker_args));
                args->filepath = strdup(full_path); args->entry = te; args->ctx = &ctx;
                pthread_mutex_lock(&ctx.lock); ctx.tasks_remaining++; pthread_mutex_unlock(&ctx.lock);
                threadpool_add(pool, process_file_task, args);
                should_add = 1;
            } else {
                size_t sz; char *c = read_file_to_buffer(full_path, &sz);
                if (c) {
                    write_object(c, sz, "blob", NULL, te->sha1);
                    free(c); should_add = 1;
                } else {
                    free(te->name); free(te);
                }
            }
        }

        if (should_add) ctx.entries[ctx.count++] = te;
    }
    closedir(d);

    pthread_mutex_lock(&ctx.lock);
    while (ctx.tasks_remaining > 0) pthread_cond_wait(&ctx.done, &ctx.lock);
    pthread_mutex_unlock(&ctx.lock);

    if (ctx.count == 0) {
        free(ctx.entries); return 1;
    }
    
    qsort(ctx.entries, ctx.count, sizeof(struct tree_entry*), compare_entries);
    size_t total_size = 0;
    for (int i = 0; i < ctx.count; i++) 
        total_size += strlen(ctx.entries[i]->mode) + 1 + strlen(ctx.entries[i]->name) + 1 + SHA_DIGEST_LENGTH;

    char *buffer = malloc(total_size);
    char *ptr = buffer;
    for (int i = 0; i < ctx.count; i++) {
        struct tree_entry *e = ctx.entries[i];
        ptr += sprintf(ptr, "%s %s", e->mode, e->name) + 1;
        memcpy(ptr, e->sha1, SHA_DIGEST_LENGTH);
        ptr += SHA_DIGEST_LENGTH;
        free(e->name); free(e);
    }
    write_object(buffer, total_size, "tree", out_sha1_hex, out_sha1_binary);
    free(buffer); free(ctx.entries);
    
    return 0;
}
