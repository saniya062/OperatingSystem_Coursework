#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <stdbool.h>

#define PORT            8080
#define BUFFER_SIZE     1024
#define MAX_USERS       3
#define MAX_LOGIN_TRIES 3

/* ------------------------------------------------------------
   SIMPLE PROTOCOL (plain-text, newline-terminated commands)
   ------------------------------------------------------------
   Client -> Server:
       LOGIN <username> <password>   authenticate
       ECHO <message>                server echoes message back
       TIME                          ask server for current time
       QUIT                          close the connection

   Server -> Client:
       WELCOME ...
       LOGIN_OK / LOGIN_FAIL / LOGIN_LOCKOUT
       ECHO_REPLY <message>
       TIME_REPLY <time string>
       ERROR <reason>                malformed / invalid input
       BYE
   ------------------------------------------------------------ */

typedef struct {
    char username[32];
    char password[32];
} User;

User users[MAX_USERS] = {
    {"admin",  "admin123"},
    {"saniya", "securePass1"},
    {"guest",  "guest123"}
};

bool authenticate(const char *username, const char *password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return strcmp(users[i].password, password) == 0;
        }
    }
    return false;
}

/* Strips the trailing newline/carriage-return a client may send */
void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

/* Runs in its own thread for every connected client, so multiple
   clients are served concurrently instead of one at a time. */
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    getpeername(client_fd, (struct sockaddr *)&peer, &peer_len);
    inet_ntop(AF_INET, &peer.sin_addr, client_ip, sizeof(client_ip));

    printf("[SERVER] Client connected from %s\n", client_ip);

    const char *welcome = "WELCOME - please LOGIN <username> <password>\n";
    send(client_fd, welcome, strlen(welcome), 0);

    /* ---- Stage 1: authentication (basic security measure) ---- */
    bool logged_in = false;
    char logged_user[32] = "";

    for (int attempt = 0; attempt < MAX_LOGIN_TRIES && !logged_in; attempt++) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("[SERVER] Client %s disconnected before login.\n", client_ip);
            close(client_fd);
            return NULL;
        }
        buffer[n] = '\0';
        trim_newline(buffer);

        char cmd[16], user[32], pass[32];
        int fields = sscanf(buffer, "%15s %31s %31s", cmd, user, pass);

        /* DATA VALIDATION: reject malformed LOGIN requests instead
           of letting a bad packet crash or confuse the server */
        if (fields != 3 || strcmp(cmd, "LOGIN") != 0) {
            const char *err = "ERROR malformed LOGIN, expected: LOGIN <user> <pass>\n";
            send(client_fd, err, strlen(err), 0);
            continue;
        }

        if (authenticate(user, pass)) {
            logged_in = true;
            strncpy(logged_user, user, sizeof(logged_user) - 1);
            const char *ok = "LOGIN_OK\n";
            send(client_fd, ok, strlen(ok), 0);
            printf("[SERVER] %s authenticated as '%s'.\n", client_ip, user);
        } else {
            const char *fail = "LOGIN_FAIL\n";
            send(client_fd, fail, strlen(fail), 0);
            printf("[SERVER] %s failed login attempt %d/%d.\n", client_ip, attempt + 1, MAX_LOGIN_TRIES);
        }
    }

    if (!logged_in) {
        const char *lock = "LOGIN_LOCKOUT\n";
        send(client_fd, lock, strlen(lock), 0);
        printf("[SERVER] %s locked out after %d failed attempts.\n", client_ip, MAX_LOGIN_TRIES);
        close(client_fd);
        return NULL;
    }

    /* ---- Stage 2: command loop ---- */
    while (1) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            /* ERROR HANDLING: client dropped the connection unexpectedly */
            printf("[SERVER] %s (%s) disconnected.\n", client_ip, logged_user);
            break;
        }
        buffer[n] = '\0';
        trim_newline(buffer);

        if (strncmp(buffer, "ECHO ", 5) == 0) {
            char reply[BUFFER_SIZE];
            snprintf(reply, sizeof(reply), "ECHO_REPLY %s\n", buffer + 5);
            send(client_fd, reply, strlen(reply), 0);

        } else if (strcmp(buffer, "TIME") == 0) {
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            char reply[BUFFER_SIZE];
            snprintf(reply, sizeof(reply), "TIME_REPLY %s\n", timestamp);
            send(client_fd, reply, strlen(reply), 0);

        } else if (strcmp(buffer, "QUIT") == 0) {
            const char *bye = "BYE\n";
            send(client_fd, bye, strlen(bye), 0);
            printf("[SERVER] %s (%s) quit gracefully.\n", client_ip, logged_user);
            break;

        } else {
            /* DATA VALIDATION: unknown/malformed command */
            const char *err = "ERROR unknown command\n";
            send(client_fd, err, strlen(err), 0);
        }
    }

    close(client_fd);
    return NULL;
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /* allows quick restart of the server without "address already in use" */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("=== Multi-threaded Chat/Command Server ===\n");
    printf("Listening on port %d...\n\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (*client_fd < 0) {
            perror("accept failed");
            free(client_fd);
            continue; /* don't let one bad accept kill the whole server */
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);
        pthread_detach(tid); /* thread cleans up its own resources when done */
    }

    close(server_fd);
    return 0;
}
