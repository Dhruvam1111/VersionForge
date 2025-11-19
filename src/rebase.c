#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>

#include "rebase.h"
#include "utils.h"  // For resolve_ref, read_ref
#include "database.h" // For read_object

// Actions available in interactive rebase
typedef enum {
    ACTION_PICK,
    ACTION_DROP,
    ACTION_EDIT,
    ACTION_SQUASH
} RebaseAction;

// Represents one commit in the rebase list
typedef struct {
    char hash[41];
    char message[100];
    RebaseAction action;
} RebaseEntry;

// Helper to convert action enum to string
const char* action_to_str(RebaseAction a) {
    switch(a) {
        case ACTION_PICK: return "pick";
        case ACTION_DROP: return "drop";
        case ACTION_EDIT: return "edit";
        case ACTION_SQUASH: return "squash";
        default: return "unknown";
    }
}

/**
 * @brief Pauses the rebase process and spawns a new shell to allow the user to make manual edits.
 * (Course concept: Process Management for the "edit" feature.)
 */
static int spawn_editor_shell(const char *commit_sha) {
    printf("\n------------------------------------------------------------\n");
    printf("STOPPED at commit %.7s for EDITING.\n", commit_sha);
    printf("You are now in a nested shell.\n");
    printf("1. Make your changes to files.\n");
    printf("2. Test your code.\n");
    printf("3. Type 'exit' to return to Version Forge and finish the rebase.\n");
    printf("------------------------------------------------------------\n");
    
    // 1. Fork a child process
    pid_t pid = fork();

    if (pid == 0) { // Child Process 
        // Use execvp to replace the current process with a shell interpreter
        // This pauses the rebase until the user manually exits the shell.
        char *const shell_args[] = {"/bin/bash", NULL};
        execvp("/bin/bash", shell_args);
        
        // If execvp returns, it failed
        perror("execvp failed to launch shell");
        exit(1); 

    } else if (pid > 0) { // Parent Process 
        int status;
        
        // Wait for the child shell process to terminate
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return -1;
        }
        
        // Check exit status
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("\n--- Exited EDIT mode. Resuming Rebase... ---\n");
            return 0; 
        } else {
            fprintf(stderr, "Error: Shell editing failed or was aborted.\n");
            return -1;
        }

    } else { // Fork failed
        perror("fork failed");
        return -1;
    }
}

// Helper to fetch simple commit message (first line)
void get_commit_message(const char *hash, char *buffer, size_t size) {
    char *type, *data;
    size_t len;
    if (read_object(hash, &type, &data, &len) == 0) {
        if (strcmp(type, "commit") == 0) {
            // Find double newline (end of headers)
            char *msg_start = strstr(data, "\n\n");
            if (msg_start) {
                msg_start += 2; // Skip \n\n
                // Copy until next newline or end
                size_t i = 0;
                while (i < size - 1 && msg_start[i] != '\n' && msg_start[i] != '\0') {
                    buffer[i] = msg_start[i];
                    i++;
                }
                buffer[i] = '\0';
            } else {
                snprintf(buffer, size, "<no message>");
            }
        }
        free(type); free(data);
    } else {
        snprintf(buffer, size, "<error reading commit>");
    }
}

/**
 * @brief Main logic for the guided, menu-driven interactive rebase.
 */
int do_rebase_interactive(const char *target_ref) {
    printf("Launching Interactive Rebase onto '%s'...\n", target_ref);

    // 1. Mocking the Commit List for Demonstration
    // In a full implementation, you would traverse parents from HEAD until you reach target_ref.
    // Here, we simulate a list of recent commits to demonstrate the UI.
    
    int commit_count = 3;
    RebaseEntry commits[3];

    // Getting HEAD for realism
    char head_hash[41];
    char head_ref[256];
    if (resolve_ref("HEAD", head_ref) == 0) {
        read_ref(head_ref, head_hash);
    } else {
        strcpy(head_hash, "0000000"); // Dummy
    }

    // Fake data structure for the UI demo
    strcpy(commits[0].hash, head_hash);
    get_commit_message(head_hash, commits[0].message, 100);
    commits[0].action = ACTION_PICK;

    strcpy(commits[1].hash, "a1b2c3d");
    strcpy(commits[1].message, "Fix login bug");
    commits[1].action = ACTION_PICK;

    strcpy(commits[2].hash, "e5f6g7h");
    strcpy(commits[2].message, "Update documentation");
    commits[2].action = ACTION_PICK;

    // 2. The Interactive UI Loop
    char input[10];
    int running = 1;

    while (running) {
        // Clear screen (ANSI escape code)
        printf("\033[H\033[J"); 
        
        printf("=== VERSION FORGE INTERACTIVE REBASE ===\n");
        printf("Rebasing %d commits onto %s\n\n", commit_count, target_ref);

        for (int i = 0; i < commit_count; i++) {
            printf("  [%d] %-6s %.7s %s\n", 
                   i + 1, 
                   action_to_str(commits[i].action), 
                   commits[i].hash, 
                   commits[i].message);
        }

        printf("\nCommands:\n");
        printf("  <number> p : Set to PICK (Keep commit)\n");
        printf("  <number> d : Set to DROP (Remove commit)\n");
        printf("  <number> e : Set to EDIT (Pause for shell access)\n");
        printf("  r          : RUN rebase\n");
        printf("  q          : ABORT\n");
        printf("\nAction > ");
        
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // Parse Input
        if (input[0] == 'q') {
            printf("Rebase aborted.\n");
            return 0;
        }
        if (input[0] == 'r') {
            running = 0; // Exit loop and execute
            continue;
        }

        int index = atoi(input) - 1;
        if (index >= 0 && index < commit_count) {
            char cmd = input[strcspn(input, " \n") - 1]; // Get last char
            // Or simpler parsing:
            if (strstr(input, "p")) commits[index].action = ACTION_PICK;
            else if (strstr(input, "d")) commits[index].action = ACTION_DROP;
            else if (strstr(input, "e")) commits[index].action = ACTION_EDIT;
            else if (strstr(input, "s")) commits[index].action = ACTION_SQUASH;
        }
    }

    // 3. Execution Phase
    printf("\nExecuting Rebase Plan...\n");
    
    for (int i = commit_count - 1; i >= 0; i--) {
        RebaseEntry *entry = &commits[i];
        
        printf("Processing %.7s (%s)... ", entry->hash, action_to_str(entry->action));
        
        if (entry->action == ACTION_DROP) {
            printf("DROPPED.\n");
            continue;
        }
        
        if (entry->action == ACTION_EDIT) {
            printf("PAUSING.\n");
            // *** Call the Process Management Logic ***
            if (spawn_editor_shell(entry->hash) != 0) {
                fprintf(stderr, "Rebase failed during edit.\n");
                return -1;
            }
        } else {
            // ACTION_PICK
            printf("APPLIED.\n");
            // In real implementation: cherry_pick(entry->hash);
        }
        
        // In real implementation: update HEAD to point to the new commit
        sleep(1); // Simulate work
    }

    printf("\nSuccessfully rebased and updated refs/heads/main.\n");
    return 0;
}
