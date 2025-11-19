CC = gcc
CFLAGS = -g -Wall -Iinclude
LDFLAGS = -lz -lcrypto -lpthread

# All sources
SRCS = $(wildcard src/*.c)
ALL_OBJS = $(SRCS:.c=.o)

# CLIENT objects: Everything EXCEPT server.c
CLIENT_OBJS = $(filter-out src/server.o, $(ALL_OBJS))

# SERVER objects: Everything EXCEPT main.c AND network_client.c
# The server needs network_utils.o, but not the client command logic.
SERVER_OBJS = $(filter-out src/main.o src/network_client.o, $(ALL_OBJS))

all: version_forge vf_server

version_forge: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o version_forge $(CLIENT_OBJS) $(LDFLAGS)

vf_server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o vf_server $(SERVER_OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o version_forge vf_server

.PHONY: all clean
