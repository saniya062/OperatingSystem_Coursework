#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256

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

int main(void) {
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

    return 0;
}
