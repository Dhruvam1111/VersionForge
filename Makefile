# Compiler
CC = gcc

# Compiler flags
# -g: Adds debugging information
# -Wall: Enables all warnings
# -Iinclude: Tells gcc to look for header files in the 'include' directory
CFLAGS = -g -Wall -Iinclude

# Linker flags
# -lz: Links the zlib library (for compression)
# -lcrypto: Links the OpenSSL crypto library (for SHA-1)
LDFLAGS = -lz -lcrypto

# Find all .c files in the src directory
SRCS = $(wildcard src/*.c)

# Replace .c extensions with .o (object files)
OBJS = $(SRCS:.c=.o)

# The final executable name
TARGET = version_forge

# Default target: build the 'version_forge' executable
all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile .c source files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean
