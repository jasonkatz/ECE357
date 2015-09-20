#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {

    // Start the clock
    clock_t start = clock(), diff;

    int i;

    /*
     * Get options
     */
    char * outfile = NULL;
    int bufferSize = 4096; // Default buffer size 4kb
    char ** inputFiles = argv;
    int numToRemove = 1;

    // Start reading args after run command
    for (i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            outfile = argv[i + 1];
            // Set var to remove args from input array
            numToRemove += 2;
            ++i;
        } else if (strcmp(argv[i], "-b") == 0) {
            bufferSize = atoi(argv[i + 1]);
            // Set var to remove args from input array
            numToRemove += 2;
            ++i;
        }
    }

    // Remove args from input array
    int numInputFiles = argc - numToRemove;
    for (i = 0; i < numInputFiles; ++i) {
        inputFiles[i] = inputFiles[i + numToRemove];
    }
    if (numInputFiles == 0) {
        ++numInputFiles;
        inputFiles[0] = "-";
    }

    /*
     * Read/write
     */

    // Open the output file (STDOUT if none defined)
    int outfd;
    if (!outfile) {
        outfd = STDOUT_FILENO;
    } else {
        outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    }

    // Loop through input files
    for (i = 0; i < numInputFiles; ++i) {
        // Open file
        int infd = open(inputFiles[i], O_RDONLY, 777);
        if (strcmp(inputFiles[i], "-") == 0) {
            infd = STDIN_FILENO;
        }
        if (infd == -1) {
            printf("File read error (%s)\n", inputFiles[i]);
            perror("");
            return -1;
        }
        // Read from each file
        int byteNum;
        char buf[bufferSize];
        while ((byteNum = read(infd, buf, bufferSize)) > 0) {
            if (write(outfd, buf, byteNum) != byteNum) {
                perror("write error");
                return -1;
            }
        }

        if (byteNum < 0) {
            perror("read error");
            return -1;
        }
    }

    // Close file
    int closeStatus = close(outfd);

    if (closeStatus == -1) {
        printf("error: %d\t%s\n", errno, strerror(errno));
    }

    // Stop the clock
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %d seconds %d milliseconds\n", msec / 1000, msec % 1000);

    return 0;
}
