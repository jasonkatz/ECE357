#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char ** argv) {

    int i;
    char * outfile = NULL;
    int bufferSize = 4096; // Default buffer size 4kb
    char ** inputFiles = argv;
    int numInputFiles = argc - 2; // First two elements of argv are command string and pattern

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

    int outfd = STDOUT_FILENO;

    // Loop through input files
    for (i = 0; i < numInputFiles; ++i) {
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
                perror("write error");
                exit(1);
            } else if (wrstatus < byteNum) {
                // Write to stdout and retry write on partial write
                printf("Partial write to buffer\n");
                write(outfd, buf, byteNum - wrstatus);
            }
        }

        if (byteNum < 0) {
            fprintf(stderr, "Read error on file %s: %s\n", inputFiles[i], strerror(errno));
            exit(1);
        }
    }

    // Close file
    int closeStatus = close(outfd);

    if (closeStatus == -1) {
        printf("error: %d\t%s\n", errno, strerror(errno));
    }

    return 0;
}
