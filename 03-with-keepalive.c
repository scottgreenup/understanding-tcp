#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
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

    // Turn on keep alive and set some reasonable settings
    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0) {
        perror("setsockopt on SOL_SOCKET::SO_KEEPALIVE failed");
        close(sock);
        return 1;
    }

    // Delay before we start keep alive
    #ifdef TCP_KEEPIDLE // Linux
    int idle_seconds = 10;
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle_seconds, sizeof(idle_seconds)) < 0) {
        perror("setsockopt on IPPROTO_TCP::TCP_KEEPIDLE failed");
        close(sock);
        return 1;
    }
    #elif defined(TCP_KEEPALIVE) // Darwin
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, &idle_seconds, sizeof(idle_seconds)) < 0) {
        perror("setsockopt on IPPROTO_TCP::TCP_KEEPALIVE failed");
        close(sock);
        return 1;
    }
    #endif

    // Delay between each keep alive packet
    #ifdef TCP_KEEPINTVL
    int interval_seconds = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval_seconds, sizeof(interval_seconds)) < 0) {
        perror("setsockopt on IPPROTO_TCP::TCP_KEEPINTVL failed");
        close(sock);
        return 1;
    }
    #else
    fprintf(stderr, "Unable to set TCP_KEEPINTVL\n");
    #endif

    // How many we send before we determine the conn is dead
    #ifdef TCP_KEEPCNT
    int count = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count)) < 0) {
        perror("setsockopt on IPPROTO_TCP::TCP_KEEPCNT failed");
        close(sock);
        return 1;
    }
    #else
    fprintf(stderr, "Unable to set TCP_KEEPCNT\n");
    #endif

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
