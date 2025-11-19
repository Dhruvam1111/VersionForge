#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 

#include "init.h"
#include "database.h"
#include "utils.h"
#include "commit.h"
#include "log.h"
#include "status.h"
#include "checkout.h"
#include "branch.h"
#include "vf_signals.h"
#include "network_client.h"
#include "merge.h"
#include "rebase.h" 
#include "config.h" 

int main(int argc, char *argv[]) {
    // 1. Setup Signal Handling
    if (vf_client_signal_setup() != 0) {
        fprintf(stderr, "Warning: Failed to setup signal handlers.\n");
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  init\n");
        fprintf(stderr, "  config --global <key> <value>\n");              
        fprintf(stderr, "  commit -m <msg>\n");
        fprintf(stderr, "  log\n");
        fprintf(stderr, "  status\n");
        fprintf(stderr, "  checkout <branch/hash>\n");
        fprintf(stderr, "  branch <name>\n");
        fprintf(stderr, "  merge <branch>\n");
        fprintf(stderr, "  rebase -i <branch>\n");
        fprintf(stderr, "  push\n");
        fprintf(stderr, "  pull\n");
        fprintf(stderr, "  fork\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "init") == 0) {
        return do_init();
    } 
    else if (strcmp(command, "config") == 0) {
        if (argc < 4 || strcmp(argv[2], "--global") != 0) {
            fprintf(stderr, "Usage: %s config --global <key> <value>\n", argv[0]);
            return 1;
        }
        // e.g. argv[3] = "user.name", argv[4] = "John Doe"
        if (argc >= 5) {
            return do_config(argv[3], argv[4]);
        } else {
            fprintf(stderr, "Missing value.\n");
            return 1;
        }
    }
    else if (strcmp(command, "hash-object") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s hash-object <file>\n", argv[0]);
            return 1;
        }
        char *filepath = argv[2];
        size_t file_size = 0;
        char *file_content = read_file_to_buffer(filepath, &file_size);
        if (file_content == NULL) {
            fprintf(stderr, "Error: Could not read file '%s'\n", filepath);
            return 1;
        }
        char sha1_hex[41];
        if (write_object(file_content, file_size, "blob", sha1_hex, NULL) != 0) {
            fprintf(stderr, "Error: Could not write blob object for '%s'\n", filepath);
            free(file_content);
            return 1;
        }
        printf("%s\n", sha1_hex);
        free(file_content);
    } 
    else if (strcmp(command, "commit") == 0) {
        if (argc < 4 || strcmp(argv[2], "-m") != 0) {
            fprintf(stderr, "Usage: %s commit -m \"<message>\"\n", argv[0]);
            return 1;
        }
        return do_commit(argv[3]);
    }
    else if (strcmp(command, "log") == 0) {
        return do_log();
    }
    else if (strcmp(command, "status") == 0) {
        return do_status();
    }
    else if (strcmp(command, "checkout") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s checkout <hash>\n", argv[0]);
            return 1;
        }
        return do_checkout(argv[2]);
    }
    else if (strcmp(command, "branch") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s branch <new-branch-name>\n", argv[0]);
            return 1;
        }
        return do_branch(argv[2]);
    }
    else if (strcmp(command, "push") == 0) {
        return do_push();
    }
    else if (strcmp(command, "pull") == 0) {
        return do_pull();
    }
    else if (strcmp(command, "fork") == 0) {
        return do_fork();
    }
    else if (strcmp(command, "merge") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s merge <branch>\n", argv[0]);
            return 1;
        }
        return do_merge(argv[2]);
    }
    else if (strcmp(command, "rebase") == 0) {
        if (argc < 4 || strcmp(argv[2], "-i") != 0) {
            fprintf(stderr, "Usage: %s rebase -i <target_branch>\n", argv[0]);
            return 1;
        }
        return do_rebase_interactive(argv[3]);
    }
    else if (strcmp(command, "test-signals") == 0) {
        printf("Running signal test... (Press Ctrl+C to stop)\n");
        while (!shutdown_requested) {
            printf("Waiting for signal...\n");
            sleep(1); 
        }
        printf("\nSuccess! Shutdown signal received. Exiting gracefully.\n");
        return 0;
    }
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        return 1;
    }

    return 0;
}
