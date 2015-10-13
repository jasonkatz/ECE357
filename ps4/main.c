#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

jmp_buf int_jb;

char * commands[2] = { "grep", "less" };
int i;
int bufferSize = 4096; // Default buffer size 4kb

void pipeFile(char * fileName, char * pattern, int * totalBytes, int * totalFiles);

void int_handler(int sn) {
    longjmp(int_jb, 1);
}

int main(int argc, char ** argv) {
    (void)signal(SIGINT, int_handler);

    char * pattern = argv[1];

    char ** inputFiles = argv;
    int numInputFiles = argc - 2; // First two elements of argv are command string and pattern

    // If there are fewer than 3 elements in argv, something's wrong
    if (argc < 3) {
        fprintf(stderr, "Usage: catgrepmore pattern infile1 [...infile2...]\n");
        exit(1);
    }

    // Remove command string and pattern from inputFiles
    for (i = 0; i < numInputFiles; ++i) {
        inputFiles[i] = inputFiles[i + 2];
    }

    // Statistics
    int totalFiles = 0;
    int totalBytes = 0;

    /*
     * Read/write
     */

    // Loop through input files
    for (i = 0; i < numInputFiles; ++i) {
        pipeFile(inputFiles[i], pattern, &totalBytes, &totalFiles);
    }

    return 0;
}

void pipeFile(char * fileName, char * pattern, int * totalBytes, int * totalFiles) {
    ++(*totalFiles);

    pid_t pid1;
    pid_t pid2;
    int status1;
    int status2;
    struct rusage ru1;
    struct rusage ru2;

    if (setjmp(int_jb) != 0) {
        fprintf(stderr, "Interruped!\nTotal files processed: %d\nTotal bytes processed: %d\n", (*totalFiles), (*totalBytes));
        fprintf(stderr, "Interrupted!\n");
        exit(1);
    }

    // Create pipe 1
    int pipe1fds[2], pipe2fds[2];
    if (pipe(pipe1fds) < 0) {
        fprintf(stderr, "Can't create pipe 1 for input file %s: %s\n", fileName, strerror(errno));
        exit(1);
    }

    // Create pipe 2
    if (pipe(pipe2fds) < 0) {
        fprintf(stderr, "Can't create pipe 2 for input file %s: %s\n", fileName, strerror(errno));
        exit(1);
    }

    if (setjmp(int_jb) != 0) {
        fprintf(stderr, "Interruped!\nTotal files processed: %d\nTotal bytes processed: %d\n", (*totalFiles), (*totalBytes));
        fprintf(stderr, "Interrupted!\n");
        exit(1);
    }

    // Fork process to exec grep
    switch(pid1 = fork()) {
        case 0: {
            // Close dangling file descriptors
            close(pipe1fds[1]);
            close(pipe2fds[0]);

            // Dup grep stdin to read end of pipe 1
            if (dup2(pipe1fds[0], STDIN_FILENO) < 0) {
                fprintf(stderr, "Can't dup2 stdin of grep to read end of pipe 1: %s\n", strerror(errno));
                exit(1);
            }

            // Dup grep stdout to write end of pipe 2
            if (dup2(pipe2fds[1], STDOUT_FILENO) < 0) {
                fprintf(stderr, "Can't dup2 stdout of grep to write end of pipe 2: %s\n", strerror(errno));
                exit(1);
            }

            // Close dangling file descriptors
            close(pipe1fds[0]);
            close(pipe2fds[1]);

            // Exec the child grep process
            char * args[3] = { commands[0], pattern, 0 };
            execvp(commands[0], args);
            break;
        }
        case -1: {
            fprintf(stderr, "Fork failed: %s\n", strerror(errno));
            exit(1);
            break;
        }
        default: {
            // Close dangling file descriptors
            close(pipe1fds[0]);

            // Open file
            int infd;
            if ((infd = open(fileName, O_RDONLY, 0777)) < 0) {
                fprintf(stderr, "File open error (%s): %s\n", fileName, strerror(errno));
                exit(1);
            }
            // Read from the file
            int byteNum;
            char buf[bufferSize];
            while ((byteNum = read(infd, buf, bufferSize)) > 0) {
                int wrstatus = write(pipe1fds[1], buf, byteNum);
                if (wrstatus <= 0) {
                    fprintf(stderr, "Write error on fd %d: %s\n", pipe1fds[1], strerror(errno));
                    exit(1);
                } else if (wrstatus < byteNum) {
                    // Write to stdout and retry write on partial write
                    printf("Partial write from buffer to stdout\n");
                    write(pipe1fds[1], buf, byteNum - wrstatus);
                }
                (*totalBytes) += wrstatus;
            }

            if (byteNum < 0) {
                fprintf(stderr, "Read error on file %s: %s\n", fileName, strerror(errno));
                exit(1);
            }

            // Close output file
            close(pipe1fds[1]);

            if (wait3(&status1, 0, &ru1) < 0) {
                fprintf(stderr, "Wait3 error: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
    }

    // Fork process to exec more
    switch(pid2 = fork()) {
        case 0: {
            // Close dangling file descriptor
            close(pipe2fds[1]);

            // Dup grep stdin to read end of pipe 2
            if (dup2(pipe2fds[0], STDIN_FILENO) < 0) {
                fprintf(stderr, "Can't dup2 stdin of more to read end of pipe 2: %s\n", strerror(errno));
                exit(1);
            }

            // Close dangling file descriptor
            close(pipe1fds[0]);

            // Exec the child grep process
            char * args[3] = { commands[1], 0 };
            execvp(commands[1], args);
            break;
        }
        case -1: {
            fprintf(stderr, "Fork failed: %s\n", strerror(errno));
            exit(1);
            break;
        }
        default: {
            close(pipe2fds[0]);
            close(pipe2fds[1]);

            if (wait3(&status2, 0, &ru2) < 0) {
                fprintf(stderr, "Wait3 error: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
    }
}
