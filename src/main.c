#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "init.h"
#include "database.h"
#include "utils.h"
#include "commit.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "init") == 0) {
        return do_init();
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
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        return 1;
    }

    return 0;
}
