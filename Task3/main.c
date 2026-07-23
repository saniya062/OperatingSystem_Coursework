#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE 256
#define MAX_USERS          3
#define USERNAME_LEN       32
#define PASSWORD_LEN       32
#define MAX_LOGIN_ATTEMPTS 3
/* ------------------------------------------------------------
   PART 1: FILE CRUD OPERATIONS (Create, Read, Write, Delete)
   ------------------------------------------------------------ */

/* CREATE: makes a new file and writes initial content into it */
int create_file(const char *filename, const char *content) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("[CREATE] Failed to create '%s'.\n", filename);
        return -1;
    }
    fprintf(fp, "%s", content);
    fclose(fp);
    printf("[CREATE] '%s' created successfully.\n", filename);
    return 0;
}

/* READ: opens a file and prints its contents to the screen */
int read_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("[READ] '%s' does not exist.\n", filename);
        return -1;
    }

    char line[MAX_LINE];
    printf("[READ] Contents of '%s':\n", filename);
    printf("----------------------------------------\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    printf("\n----------------------------------------\n");
    fclose(fp);
    return 0;
}

/* WRITE (append): adds more content to the end of an existing file
   without erasing what was already there */
int append_to_file(const char *filename, const char *content) {
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("[WRITE] Failed to open '%s' for writing.\n", filename);
        return -1;
    }
    fprintf(fp, "%s", content);
    fclose(fp);
    printf("[WRITE] Appended to '%s' successfully.\n", filename);
    return 0;
}

/* DELETE: removes a file from disk */
int delete_file(const char *filename) {
    if (remove(filename) == 0) {
        printf("[DELETE] '%s' deleted successfully.\n", filename);
        return 0;
    } else {
        printf("[DELETE] Failed to delete '%s' (may not exist).\n", filename);
        return -1;
    }
}

void run_crud_demo(void) {
    const char *filename = "sample.txt";

    printf("=== PART 1: File CRUD Operations ===\n\n");

    create_file(filename, "This is the first line.\n");
    read_file(filename);

    append_to_file(filename, "This line was appended afterwards.\n");
    read_file(filename);

    delete_file(filename);

    /* demonstrate proper error handling: reading a file
       that no longer exists after deletion */
    read_file(filename);

    printf("/n");
}
/* ------------------------------------------------------------
   PART 2: USER AUTHENTICATION MECHANISM
   ------------------------------------------------------------
   A simple in-memory "user database". In a production system
   passwords would be salted and hashed (e.g. SHA-256) rather
   than stored in plain text - noted here as a security
   consideration for the report, kept simple for this demo. */
typedef struct {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
} User;

User users[MAX_USERS] = {
    {"admin", "admin123"},
    {"saniya", "securePass1"},
    {"guest",  "guest123"}
};

/* Looks up a username and checks the password against it.
   Returns true only if both the username exists AND the
   password matches exactly. */
bool authenticate(const char *username, const char *password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return strcmp(users[i].password, password) == 0;
        }
    }
    return false; /* username not found */
}

/* Simulates a login session: gives the user up to
   MAX_LOGIN_ATTEMPTS tries before locking them out.
   This models basic brute-force protection. */
bool login_session(const char *username, const char *attempts[], int num_attempts) {
    for (int i = 0; i < num_attempts && i < MAX_LOGIN_ATTEMPTS; i++) {
        printf("[LOGIN] Attempt %d/%d for user '%s' with password '%s'... ",
               i + 1, MAX_LOGIN_ATTEMPTS, username, attempts[i]);

        if (authenticate(username, attempts[i])) {
            printf("SUCCESS\n");
            return true;
        } else {
            printf("FAILED\n");
        }
    }
    printf("[LOGIN] Account locked - too many failed attempts for '%s'.\n", username);
    return false;
}

void run_auth_demo(void) {
    printf("=== PART 2: User Authentication ===\n\n");

    const char *correct_attempt[] = {"securePass1"};
    login_session("saniya", correct_attempt, 1);
    printf("\n");

    const char *mixed_attempts[] = {"wrongpass", "admin123"};
    login_session("admin", mixed_attempts, 2);
    printf("\n");

    const char *bad_attempts[] = {"wrong1", "wrong2", "wrong3"};
    login_session("guest", bad_attempts, 3);
    printf("\n");

    const char *nobody_attempts[] = {"whatever"};
    login_session("nobody", nobody_attempts, 1);
    printf("\n");
}

/* ------------------------------------------------------------
   MAIN
   ------------------------------------------------------------ */
int main(void) {
    run_crud_demo();
    run_auth_demo();
    return 0;
}
