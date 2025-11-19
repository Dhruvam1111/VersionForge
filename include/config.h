#ifndef CONFIG_H
#define CONFIG_H

// Set a global config value (e.g., user.name "John")
int do_config(const char *key, const char *value);

// Get a config value. Returns 0 if found, -1 if not.
// buffer should be large enough (e.g., 256 bytes).
int get_config_value(const char *key, char *buffer, size_t size);

#endif
