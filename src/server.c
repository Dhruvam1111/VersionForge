#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "network.h"
#include "vf_signals.h" 
#include "network_utils.h" // <--- INCLUDE THIS

#define BUFFER_SIZE 1024

// ... [send_object_file, scan_and_send, receive_object_file REMOVED from here] ...

void handle_client(int client_socket) {
    // ... (Keep your existing handle_client logic exactly as it was) ...
    // It will now call the functions from network_utils.c
    
    char buffer[BUFFER_SIZE];
    int bytes_read;

    struct timeval tv;
    tv.tv_sec = 5; tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0; 

        printf("[Server] Received: '%s'\n", buffer);
        fflush(stdout);

        if (strncmp(buffer, CMD_HELLO, strlen(CMD_HELLO)) == 0) {
            write(client_socket, "VF_SERVER_V1\n", 13);
        } 
        else if (strncmp(buffer, CMD_PUSH, strlen(CMD_PUSH)) == 0) {
            printf("[Server] Processing PUSH request...\n");
            fflush(stdout);
            write(client_socket, "PUSH_ACCEPTED\n", 14);
            
            while(1) {
                memset(buffer, 0, 1024);
                if (read(client_socket, buffer, 1024) <= 0) break;
                if (strncmp(buffer, "END", 3) == 0) break; 
                
                char hash[41];
                int size;
                sscanf(buffer, "OBJ %s %d", hash, &size);
                write(client_socket, "ACK\n", 4);
                
                // Call shared function
                receive_object_file(client_socket, hash, size);
                
                write(client_socket, "SAVED\n", 6);
            }
            printf("[Server] Push complete.\n");
            break; 
        } 
        else if (strncmp(buffer, CMD_PULL, strlen(CMD_PULL)) == 0) {
            printf("[Server] Processing PULL request...\n");
            fflush(stdout);
            
            // Call shared function
            scan_and_send(client_socket, ".minivcs/objects");
            
            send(client_socket, "END", 3, 0);
            printf("[Server] Pull complete.\n");
            break;
        }
        else if (strncmp(buffer, CMD_FORK, strlen(CMD_FORK)) == 0) {
            printf("[Server] Initiating Real Repository Fork...\n");
            fflush(stdout);
            pid_t worker_pid = fork();
            if (worker_pid == 0) {
                printf("[Worker %d] Copying .minivcs to .minivcs_fork...\n", getpid());
                int status = system("cp -r .minivcs .minivcs_fork");
                if (status == 0) printf("[Worker %d] Fork successful.\n", getpid());
                else printf("[Worker %d] Fork failed.\n", getpid());
                fflush(stdout);
                exit(0);
            } else {
                waitpid(worker_pid, NULL, 0);
                write(client_socket, "FORK_STARTED (Worker Spawned)\n", 30);
            }
            break;
        }
        else {
            if (strlen(buffer) > 0) write(client_socket, "UNKNOWN\n", 8);
        }
    }
    
    printf("[Server] Session closed.\n");
    fflush(stdout);
    close(client_socket);
}

// ... (main function remains the same) ...
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if (vf_server_signal_setup() != 0) {
        fprintf(stderr, "Failed to setup signals.\n");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Starting Version Forge Server on Port %d...\n", VF_PORT);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct linger sl;
    sl.l_onoff = 1;
    sl.l_linger = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(VF_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (!shutdown_requested) {
        printf("[Server] Waiting for connections...\n");
        fflush(stdout);

        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        printf("[Server] New connection accepted from %s\n", inet_ntoa(address.sin_addr));
        fflush(stdout);

        handle_client(new_socket);
    }

    printf("[Server] Shutting down.\n");
    close(server_fd);
    return 0;
}
