#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char ** argv) {

    char * commands[2] = { "grep", "more" };

    int i;
    int bufferSize = 4096; // Default buffer size 4kb

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

    /*
     * Read/write
     */

    // Loop through input files
    for (i = 0; i < numInputFiles; ++i) {
        // Create pipes
        int pipe1fds[2], pipe2fds[2];
        if (pipe(pipe1fds) < 0) {
            fprintf(stderr, "Can't create pipe 1 for input file %s: %s\n", inputFiles[i], strerror(errno));
            exit(1);
        }
        if (pipe(pipe2fds) < 0) {
            fprintf(stderr, "Can't create pipe 2 for input file %s: %s\n", inputFiles[i], strerror(errno));
            exit(1);
        }

        int outfd = pipe1fds[0];

        // Fork process to exec grep and more
        pid_t pid1;
        int status1;
        struct rusage ru1;
        switch(pid1 = fork()) {
            case 0: {
                // Dup grep stdin to write end of pipe1
                if (dup2(pipe1fds[1], STDIN_FILENO) < 0) {
                    fprintf(stderr, "Can't dup2 stdin of grep to write end of pipe 1: %s\n", strerror(errno));
                    exit(1);
                }

                // Close dangling file descriptor
                close(pipe1fds[1]);

                // Exec the child grep process
                char * args[2] = { commands[0], argv[1] };
                execvp(commands[0], args);
                break;
            }
            case -1: {
                fprintf(stderr, "Fork failed: %s\n", strerror(errno));
                exit(1);
                break;
            }
            default: {
                // Open file
                int infd;
                if ((infd = open(inputFiles[i], O_RDONLY, 0777)) < 0) {
                    fprintf(stderr, "File open error (%s): %s\n", inputFiles[i], strerror(errno));
                    exit(1);
                }
                // Read from each file
                int byteNum;
                char buf[bufferSize];
                while ((byteNum = read(infd, buf, bufferSize)) > 0) {
                    int wrstatus = write(outfd, buf, byteNum);
                    if (wrstatus <= 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno));
                        exit(1);
                    } else if (wrstatus < byteNum) {
                        // Write to stdout and retry write on partial write
                        printf("Partial write from buffer to stdout\n");
                        write(outfd, buf, byteNum - wrstatus);
                    }
                }

                if (byteNum < 0) {
                    fprintf(stderr, "Read error on file %s: %s\n", inputFiles[i], strerror(errno));
                    exit(1);
                }

                // Close file
                int closeStatus = close(outfd);

                if (closeStatus == -1) {
                    fprintf(stderr, "File close error: %s\n", strerror(errno));
                }

                if (wait3(&status1, 0, &ru1) < 0) {
                    fprintf(stderr, "Wait3 error: %s\n", strerror(errno));
                    exit(1);
                }
                break;
            }
        }

    }

    return 0;
}
