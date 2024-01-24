/*
Project: Back-It-Up
Author: Matthew Skovorodin
Description: Creates file backups in a Linux environment, using multi-threading and recursive subdirectory handling.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>

#define MAX_LENGTH 256
#define MAX_THREADS 64

int file_count = 0;
int byte_count_total = 0;
int restore = 0;
pthread_t thread_id[MAX_THREADS];
pthread_mutex_t file_lock;

struct Node {
    char name[MAX_LENGTH];
    char path[MAX_LENGTH];
    int type;
};

int entryCompare(char *backup_entry, char *current_entry) {
    struct stat backup_stat;
    struct stat current_stat;
    stat(current_entry, &current_stat);
    int result = stat(backup_entry, &backup_stat);
    if (result == -1) {
        if (errno == ENOENT) {
            // printf("Backup file does not exist\n");
            return 0;
        } else {
            fprintf(stderr, "Failed to stat: %s\n", strerror(errno));
            exit(1);
        }
    }
    else {
        // printf("File exists\n");
        if (difftime(backup_stat.st_mtime, current_stat.st_mtime) == 0) {
            // printf("same times\n");
            return 1;
        }
        else if (difftime(backup_stat.st_mtime, current_stat.st_mtime) > 0) {
            // printf("backup file is newer\n");
            return 2;
        }
        else if (difftime(backup_stat.st_mtime, current_stat.st_mtime) < 0) {
            // printf("backup file is older\n");
            return 3;
        }
        else {
            fprintf(stderr, "Failed to stat: %s\n", strerror(errno));
            exit(1);
        }
    }
}

void *fileHandler(void *node_struct) {
    struct Node *node = (struct Node *)node_struct;
    char backup_entry[MAX_LENGTH];
    char current_entry[MAX_LENGTH];
    char backup_name[MAX_LENGTH];
    FILE *source, *destination;
    int result;
    int char_input;

    snprintf(backup_entry, sizeof(backup_entry), "%s/.backup/%s.bak", node->path, node->name);
    snprintf(current_entry, sizeof(current_entry), "%s/%s", node->path, node->name);
    snprintf(backup_name, sizeof(backup_name), "%s.bak", node->name);

    result = entryCompare(backup_entry, current_entry);
    if (result == 1 || result == 2) {
        printf("[Thread %d] %s does not need backing up\n", (int)pthread_self(), node->name);
        pthread_mutex_lock(&file_lock);
        file_count--;
        pthread_mutex_unlock(&file_lock);
        return NULL;
    } else if (result == 3) {
        printf("[Thread %d] WARNING: Overwriting %s\n", (int)pthread_self(), backup_name);
    } else {
        printf("[Thread %d] Backing up %s\n", (int)pthread_self(), node->name);
    }

    // file copy
    source = fopen(current_entry, "r");
    if (source == NULL) {
        fprintf(stderr, "Failed to open source file: %s\n", strerror(errno));
        return NULL;
    }

    destination = fopen(backup_entry, "w");
    if (destination == NULL) {
        fprintf(stderr, "Failed to create backup file: %s\n", strerror(errno));
        fclose(source);
        return NULL;
    }

    pthread_mutex_lock(&file_lock);
    int byte_count = 0;
    while ((char_input = fgetc(source)) != EOF) {
        fputc(char_input, destination);
        byte_count++;
        byte_count_total++;
    }

    printf("[Thread %d] Copied %d bytes from %s to %s\n", (int)pthread_self(), byte_count, node->name, backup_name);
    fclose(source);
    fclose(destination);

    // Set the last modified time of the backup file to match the current file
    struct stat current_entry_stat;
    stat(current_entry, &current_entry_stat);
    struct utimbuf times;
    times.actime = current_entry_stat.st_atime;
    times.modtime = current_entry_stat.st_mtime;

    utime(backup_entry, &times);

    pthread_mutex_unlock(&file_lock);

    pthread_exit(NULL);
}

void *restoreHandler(void *node_struct) {
    struct Node *node = (struct Node *)node_struct;
    char backup_entry[MAX_LENGTH];
    char current_entry[MAX_LENGTH];
    char backup_name[MAX_LENGTH];
    FILE *source, *destination;
    int result;
    int char_input;

    snprintf(backup_entry, sizeof(backup_entry), "%s/.backup/%s.bak", node->path, node->name);
    snprintf(current_entry, sizeof(current_entry), "%s/%s", node->path, node->name);
    snprintf(backup_name, sizeof(backup_name), "%s.bak", node->name);

    result = entryCompare(backup_entry, current_entry);
    if (result == 0 || result == 1 || result == 3) {
        printf("[Thread %d] %s does not need restoring\n", (int)pthread_self(), node->name);
        pthread_mutex_lock(&file_lock);
        file_count--;
        pthread_mutex_unlock(&file_lock);
        return NULL;
    } else {
        printf("[Thread %d] WARNING: Overwriting %s\n", (int)pthread_self(), node->name);
    }

    // file copy
    source = fopen(backup_entry, "r");
    if (source == NULL) {
        fprintf(stderr, "Failed to open backup file: %s\n", strerror(errno));
        return NULL;
    }

    destination = fopen(current_entry, "w");
    if (destination == NULL) {
        fprintf(stderr, "Failed to create restored file: %s\n", strerror(errno));
        fclose(source);
        return NULL;
    }

    pthread_mutex_lock(&file_lock);
    int byte_count = 0;
    while ((char_input = fgetc(source)) != EOF) {
        fputc(char_input, destination);
        byte_count++;
        byte_count_total++;
    }

    printf("[Thread %d] Copied %d bytes from %s to %s\n", (int)pthread_self(), byte_count, backup_name, node->name);
    fclose(source);
    fclose(destination);

    // Set the last modified time of the current file to match the backup file
    struct stat backup_entry_stat;
    stat(backup_entry, &backup_entry_stat);
    struct utimbuf times;
    times.actime = backup_entry_stat.st_atime;
    times.modtime = backup_entry_stat.st_mtime;

    utime(current_entry, &times);

    pthread_mutex_unlock(&file_lock);

    pthread_exit(NULL);
}

void threadHandler(struct Node *node) {
    pthread_mutex_init(&file_lock, NULL);

    if (restore) {
        if (pthread_create(&thread_id[file_count], NULL, restoreHandler, node) != 0) {
            fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
            exit(1);
        }
    } else {
        if (pthread_create(&thread_id[file_count], NULL, fileHandler, node) != 0) {
            fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
            exit(1);
        }
    }

    if (pthread_join(thread_id[file_count], NULL) != 0) {
        fprintf(stderr, "Failed to join thread: %s\n", strerror(errno));
        exit(1);
    }

    pthread_mutex_destroy(&file_lock);
    return;
}

void directoryCheck(char *path) {
    DIR *directory;
    char backup_directory[MAX_LENGTH];
    snprintf(backup_directory, sizeof(backup_directory), "%s/.backup", path);

    directory = opendir(backup_directory);
    if (directory == NULL) {
        if (mkdir(backup_directory, 0777) == -1) {
            fprintf(stderr, "Failed to create a backup directory: %s\n", strerror(errno));
            exit(1);
        }
    }

    closedir(directory);
}

void entryHandler(char *path) {
    DIR *directory;
    struct dirent *entry;
    struct Node node;

    directory = opendir(path);
    if (directory == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", strerror(errno));
        return;
    }

    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        strcpy(node.path, path);
        strcpy(node.name, entry->d_name);
        node.type = (entry->d_type == DT_REG) ? 1 : 0;

        threadHandler(&node);
        file_count++;
    }

    closedir(directory);
}

int main(int argc, char *argv[]) {
    char cwd[MAX_LENGTH];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Failed to get current working directory: %s\n", strerror(errno));
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        restore = 1;
    } else {
        directoryCheck(cwd);
    }

    entryHandler(cwd);

    printf("Successfully copied/restored %d files (%d bytes)\n", file_count, byte_count_total);
    return 0;
}
