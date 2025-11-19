#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#define CONFIG_FILE ".vfconfig"

// Helper to get the full path: /home/user/.vfconfig
void get_config_path(char *buffer, size_t size) {
    const char *home = getenv("HOME");
    if (!home) home = "."; // Fallback
    snprintf(buffer, size, "%s/%s", home, CONFIG_FILE);
}

int do_config(const char *key, const char *value) {
    char path[1024];
    get_config_path(path, sizeof(path));

    // Simple implementation: Read all lines, update if found, append if not.
    // For simplicity in this project, we will just APPEND or OVERWRITE the file 
    // with a specialized parser if needed, but let's stick to simple appending 
    // for unique keys or a basic KV parser.
    
    // To keep it robust without writing a full parser:
    // We will just append "key=value\n" to the file. 
    // When reading, we read the *last* occurrence of the key to get the latest value.
    
    FILE *f = fopen(path, "a");
    if (!f) {
        perror("Could not open config file");
        return 1;
    }
    
    fprintf(f, "%s=%s\n", key, value);
    fclose(f);
    
    printf("Config set: %s = %s\n", key, value);
    return 0;
}

int get_config_value(const char *key, char *buffer, size_t size) {
    char path[1024];
    get_config_path(path, sizeof(path));
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[1024];
    int found = 0;

    // Read file line by line
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        line[strcspn(line, "\n")] = 0;
        
        // Check if line starts with "key="
        size_t key_len = strlen(key);
        if (strncmp(line, key, key_len) == 0 && line[key_len] == '=') {
            // Found it! Copy the value.
            // We keep looping to ensure we get the *latest* value (bottom of file)
            strncpy(buffer, line + key_len + 1, size);
            buffer[size - 1] = '\0'; // Safety null
            found = 1;
        }
    }
    
    fclose(f);
    return found ? 0 : -1;
}
