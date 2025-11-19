#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "network.h"
#include "utils.h"
#include "network_utils.h" // <--- INCLUDE THIS

// ... [send_object_file, scan_and_send, receive_object_file REMOVED from here] ...

int vf_connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error"); return -1;
    }
    struct timeval tv;
    tv.tv_sec = 5; tv.tv_usec = 0; 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(VF_PORT);
    if (inet_pton(AF_INET, VF_DEFAULT_SERVER, &serv_addr.sin_addr) <= 0) return -1;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed"); return -1;
    }
    return sock;
}

int do_push() {
    printf("Connecting to server at %s:%d...\n", VF_DEFAULT_SERVER, VF_PORT);
    int sock = vf_connect_to_server();
    if (sock < 0) return 1;
    char buffer[1024] = {0};

    snprintf(buffer, sizeof(buffer), "%s\n", CMD_HELLO);
    send(sock, buffer, strlen(buffer), 0);
    read(sock, buffer, 1024); 

    snprintf(buffer, sizeof(buffer), "%s\n", CMD_PUSH);
    printf("Sending Command: PUSH\n");
    send(sock, buffer, strlen(buffer), 0);
    
    read(sock, buffer, 1024);
    printf("[Server]: %s", buffer);

    printf("Uploading objects...\n");
    
    // Call Shared Function
    scan_and_send(sock, ".minivcs/objects");

    send(sock, "END", 3, 0);
    printf("Push complete.\n");

    close(sock);
    return 0;
}

int do_pull() {
    printf("Connecting to server at %s:%d...\n", VF_DEFAULT_SERVER, VF_PORT);
    int sock = vf_connect_to_server();
    if (sock < 0) return 1;
    char buffer[1024] = {0};

    snprintf(buffer, sizeof(buffer), "%s\n", CMD_HELLO);
    send(sock, buffer, strlen(buffer), 0);
    read(sock, buffer, 1024); 

    snprintf(buffer, sizeof(buffer), "%s\n", CMD_PULL);
    printf("Sending Command: PULL\n");
    send(sock, buffer, strlen(buffer), 0);
    
    printf("Downloading objects...\n");
    while(1) {
        memset(buffer, 0, 1024);
        if (read(sock, buffer, 1024) <= 0) break;
        if (strncmp(buffer, "END", 3) == 0) break;
        
        char hash[41];
        int size;
        sscanf(buffer, "OBJ %s %d", hash, &size);
        
        write(sock, "ACK\n", 4); 
        
        // Call Shared Function
        receive_object_file(sock, hash, size);
        
        write(sock, "SAVED\n", 6); 
    }

    printf("Pull complete.\n");
    close(sock);
    return 0;
}

// ... (do_fork and perform_network_command remain same as previous robust version) ...
int perform_network_command(const char *command_str) {
    printf("Connecting to server at %s:%d...\n", VF_DEFAULT_SERVER, VF_PORT);
    int sock = vf_connect_to_server();
    if(sock < 0) return 1;
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s\n", CMD_HELLO);
    send(sock, buffer, strlen(buffer), 0);
    read(sock, buffer, 1024);
    snprintf(buffer, sizeof(buffer), "%s\n", command_str);
    printf("Sending Command: %s\n", command_str);
    send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, 1024);
    read(sock, buffer, 1024);
    printf("[Server Response]: %s\n", buffer);
    close(sock);
    return 0;
}

int do_fork() {
    printf("Connecting to server at %s:%d...\n", VF_DEFAULT_SERVER, VF_PORT);
    int sock = vf_connect_to_server();
    if(sock < 0) return 1;
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%s\n", CMD_HELLO);
    send(sock, buffer, strlen(buffer), 0);
    read(sock, buffer, 1024);

    snprintf(buffer, sizeof(buffer), "%s\n", CMD_FORK);
    printf("Sending Command: FORK\n");
    send(sock, buffer, strlen(buffer), 0);

    memset(buffer, 0, 1024);
    read(sock, buffer, 1024);
    printf("[Server Response]: %s\n", buffer);

    printf("Terminating session.\n");
    close(sock);
    return 0;
}
