#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "init.h" // Include its own header

int do_init() {
    if (mkdir(".minivcs", 0755) != 0) {
        if (errno == EEXIST) {
            fprintf(stderr, "Error: .minivcs directory already exists.\n");
        } else {
            perror("Error creating .minivcs directory");
        }
        return 1;
    }

    mkdir(".minivcs/objects", 0755);
    mkdir(".minivcs/refs", 0755);
    mkdir(".minivcs/refs/heads", 0755);

    FILE *head_file = fopen(".minivcs/HEAD", "w");
    if (head_file == NULL) {
        perror("Error creating HEAD file");
        return 1;
    }
    
    fprintf(head_file, "ref: refs/heads/main\n");
    fclose(head_file);

    printf("Initialized empty Version Forge repository in ./.minivcs/\n");
    return 0;
}
