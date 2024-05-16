#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>

#define MAX_FILES 100
#define BUFFER_SIZE 1024

void child_process(int pipe_fd) {
    char filename[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    int fd;
    
    // Read filename from parent
    ssize_t bytes_read = read(pipe_fd, filename, BUFFER_SIZE);
    if (bytes_read == -1) {
        perror("Error reading filename from parent");
        exit(EXIT_FAILURE);
    }

    // Open file
    filename[bytes_read] = '\0'; // Null terminate the string
    printf("Attempting to open file: %s\n", filename); // Debug statement
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Read name and date of birth
    char name[BUFFER_SIZE];
    char dob[BUFFER_SIZE];
    ssize_t name_bytes = read(fd, name, BUFFER_SIZE);
    ssize_t dob_bytes = read(fd, dob, BUFFER_SIZE);
    if (name_bytes == -1 || dob_bytes == -1) {
        perror("Error reading file content");
        exit(EXIT_FAILURE);
    }

    // Null terminate the strings
    name[name_bytes] = '\0';
    dob[dob_bytes] = '\0';

    // Calculate age (dummy calculation for demonstration)
    // Assuming current date is 2024-05-05
    int age = 2024 - atoi(strrchr(dob, '-') + 1);

    // Construct result string
    sprintf(result, "%s:%d\n", name, age);

    // Write result to parent
    ssize_t result_bytes = write(pipe_fd, result, strlen(result));
    if (result_bytes == -1) {
        perror("Error writing result to parent");
        exit(EXIT_FAILURE);
    }

    // Close file and pipe
    close(fd);
    close(pipe_fd);
    exit(EXIT_SUCCESS);
}

int main() {
    int pipe_fd[2];
    pid_t child_pid;
    DIR *dir;
    struct dirent *entry;
    char filenames[MAX_FILES][BUFFER_SIZE];
    int num_files = 0;

    // Get current directory
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current directory");
        exit(EXIT_FAILURE);
    }

    // Create pipe
    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Fork child process
    child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        close(pipe_fd[1]); // Close write end of pipe
        child_process(pipe_fd[0]);
    } else {
        // Parent process
        close(pipe_fd[0]); // Close read end of pipe

        // Open directory
        dir = opendir(cwd); // Open current directory
        if (dir == NULL) {
            perror("Error opening directory");
            exit(EXIT_FAILURE);
        }

        // Read filenames with ".usp" extension
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".usp") != NULL) {
                strcpy(filenames[num_files++], entry->d_name);
            }
        }

        // Pass filenames to child process
        printf("Number of files: %d\n", num_files); // Debug statement
        write(pipe_fd[1], &num_files, sizeof(int)); // Write number of files
        for (int i = 0; i < num_files; i++) {
            printf("Filename: %s\n", filenames[i]); // Debug statement
            write(pipe_fd[1], filenames[i], strlen(filenames[i]) + 1); // Include null terminator
        }

        // Close pipe
        close(pipe_fd[1]);

        // Wait for child process to finish
        wait(NULL);
    }

    return 0;
}

