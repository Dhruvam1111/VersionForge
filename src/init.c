#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "init.h" // Include its own header

int do_init() {
    // Try to create the main directory
    if (mkdir(".minivcs", 0755) != 0) {
        if (errno == EEXIST) {
            // It already exists. This is fine!
            // We just print a message and continue to ensure subdirs exist.
            printf("Reinitialized existing Version Forge repository in ./.minivcs/\n");
        } else {
            perror("Error creating .minivcs directory");
            return 1;
        }
    } else {
        printf("Initialized empty Version Forge repository in ./.minivcs/\n");
    }

    // Create subdirectories (mkdir fails silently if they already exist, which is what we want)
    mkdir(".minivcs/objects", 0755);
    mkdir(".minivcs/refs", 0755);
    mkdir(".minivcs/refs/heads", 0755);

    // Only create HEAD if it doesn't exist (don't overwrite current branch pointer)
    if (access(".minivcs/HEAD", F_OK) != 0) {
        FILE *head_file = fopen(".minivcs/HEAD", "w");
        if (head_file == NULL) {
            perror("Error creating HEAD file");
            return 1;
        }
        fprintf(head_file, "ref: refs/heads/main\n");
        fclose(head_file);
    }

    return 0;
}
