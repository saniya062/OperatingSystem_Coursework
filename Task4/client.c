#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT        8080
#define BUFFER_SIZE 1024
#define SERVER_IP   "127.0.0.1"

/* Strips the trailing newline from fgets() input */
void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

/* Sends one line to the server and prints the server's reply.
   Returns the number of bytes received, or <=0 on error/disconnect
   (used for the connection-management / error-handling requirement). */
ssize_t send_and_receive(int sock, const char *message, char *reply, size_t reply_size) {
    if (send(sock, message, strlen(message), 0) < 0) {
        perror("send failed");
        return -1;
    }

    ssize_t n = recv(sock, reply, reply_size - 1, 0);
    if (n <= 0) {
        printf("[CLIENT] Server closed the connection or an error occurred.\n");
        return n;
    }
    reply[n] = '\0';
    trim_newline(reply);
    return n;
}

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("invalid server address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* ERROR HANDLING: fail cleanly if the server isn't reachable,
       rather than crashing or hanging silently */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection to server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        printf("[CLIENT] No response from server.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    printf("[SERVER] %s", buffer);

    /* ---- Login stage ---- */
    char username[32], password[32];
    char message[BUFFER_SIZE], reply[BUFFER_SIZE];
    int logged_in = 0;

    while (!logged_in) {
        printf("Username: ");
        if (fgets(username, sizeof(username), stdin) == NULL) break;
        trim_newline(username);

        printf("Password: ");
        if (fgets(password, sizeof(password), stdin) == NULL) break;
        trim_newline(password);

        snprintf(message, sizeof(message), "LOGIN %s %s\n", username, password);

        if (send_and_receive(sock, message, reply, sizeof(reply)) <= 0) {
            close(sock);
            exit(EXIT_FAILURE);
        }
        printf("[SERVER] %s\n", reply);

        if (strcmp(reply, "LOGIN_OK") == 0) {
            logged_in = 1;
        } else if (strcmp(reply, "LOGIN_LOCKOUT") == 0) {
            printf("[CLIENT] Too many failed attempts. Closing connection.\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        /* otherwise LOGIN_FAIL - loop and try again */
    }

    /* ---- Command stage ---- */
    printf("\nLogged in! Available commands: ECHO <msg>, TIME, QUIT\n");
    while (1) {
        printf("> ");
        if (fgets(message, sizeof(message), stdin) == NULL) break;

        ssize_t r = send_and_receive(sock, message, reply, sizeof(reply));
        if (r <= 0) break; /* connection lost - exit gracefully */

        printf("[SERVER] %s\n", reply);

        if (strncmp(message, "QUIT", 4) == 0) break;
    }

    close(sock);
    printf("[CLIENT] Connection closed.\n");
    return 0;
}
