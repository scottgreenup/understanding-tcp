#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

struct sockaddr_in* create_sockaddr(const char* ip, uint16_t port) {
    struct sockaddr_in* addr = calloc(1, sizeof(struct sockaddr_in));
    if (!addr) {
        perror("memory allocation of sockaddr_in failed");
        return NULL;
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        perror("Invalid address");
        return NULL;
    }
    return addr;
}

int main() {
    printf("PID: %d\n", getpid());

    const char server_ip[] = "127.0.0.1";
    const uint64_t server_port = 8081;

    // Create the struct that contains the port and binary version of the IP
    // address of the server.
    struct sockaddr_in* addr = create_sockaddr(server_ip, server_port);
    if (!addr) {
        fprintf(stderr, "failed to create sockaddr_in\n");
        return 1;
    }

    // Creates a socket
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket creation failed");
        return 1;
    }

    // Set timeout for recieves... this will mean recv can only block for the
    // given seconds before unblocking.
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(sock);
        return 1;
    }

    // Connect the socket to the address we created earlier.
    if (connect(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("connect failed");
        close(sock);
        return 1;
    }

    // Send our message to server.
    const char message[] = "Hello, Server!\n";
    const ssize_t bytes_sent = send(sock, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("send failed");
        close(sock);
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        // Receive response from server
        char buffer[1024];
        const ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Server response: %s\n", buffer);
        } else {
            perror("recv failed");
        }
    }

    // Close socket
    close(sock);

    // Free memory
    free(addr);

    return 0;
}
