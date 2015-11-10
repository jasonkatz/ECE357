#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "fifo.h"

void test1();
void test2();

struct fifo * f;
int longs_per_writer = 100;

int main(int argc, char ** argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: ./a.out [testNum (1 or 2)]\n");
        exit(1);
    }

    if ((f = mmap(0, sizeof(struct fifo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) < 0) {
        fprintf(stderr, "Couldn't map to anonymous region: %s\n", strerror(errno));
        exit(1);
    }

    fifo_init(f);

    switch (argv[1][0]) {
        case '1':
            test1();
            break;
        case'2':
            test2();
            break;
        default:
            fprintf(stderr, "Please enter a valid test number\n");
            exit(1);
            break;
    }

    return 0;
}

void test1() {
    // Test 1: One reader and one writer
    pid_t pid;
    switch (pid = fork()) {
        case 0: {
            break;
        }
        case -1:
            fprintf(stderr, "Couldn't fork process: %s\n", strerror(errno));
            exit(1);
            break;
        default: {
            // Writer
            my_procnum = 0;
            unsigned long i;
            for (i = 0; i < longs_per_writer; ++i) {
                fifo_wr(f, i);
            }
            return;
            break;
        }
    }

    // Reader
    my_procnum = 1;
    int success = 1;
    unsigned long i;
    for (i = 0; i < longs_per_writer; ++i) {
        success &= (i == fifo_rd(f));
    }
    printf("Result: %d\n", success);
}

void test2() {
    // Test 2: One reader and many writers
    int numWriters = 56; // Must be <= 63 (due to the constraint on N_PROC)

    pid_t pid;
    switch (pid = fork()) {
        case 0: {
            // Create every writer process and set my_procnum appropriately
            int i;
            int inFork = 0;
            my_procnum = numWriters; // This is for the lingering parent process that does the last write
            for (i = 1; i < numWriters; ++i) {
                pid_t pidi;
                switch (pidi = fork()) {
                    case 0: {
                        my_procnum = i;
                        inFork = 1;
                        break;
                    }
                    case -1: {
                        fprintf(stderr, "Couldn't fork writer: %s\n", strerror(errno));
                        exit(1);
                        break;
                    }
                    default: {
                        break;
                    }
                }
                // Don't continue if we are in a forked process
                if (inFork) {
                    break;
                }
            }
            break;
        }
        case -1:
            fprintf(stderr, "Couldn't fork process: %s\n", strerror(errno));
            exit(1);
            break;
        default: {
            // Reader
            my_procnum = 0;
            unsigned long j;

            // Keep track of last number read from each writer to check order
            unsigned long trackers[numWriters];
            // Initialize all trackers to 0
            for (j = 0; j < numWriters; ++j) {
                trackers[j] = 0;
            }

            for (j = 0; j < numWriters * longs_per_writer; ++j) {
                unsigned long num = fifo_rd(f);
                int writer = (int)(num >> 24); // Decode writer id
                num -= ((unsigned long)writer << 24); // Decode number

                // Check if tracker is not the current number (failure condition), and increment the tracker
                if (trackers[writer - 1]++ != num) {
                    fprintf(stderr, "Test failed:\n");
                    fprintf(stderr, "Iteration %lu\n", j);
                    fprintf(stderr, "Writer %d: Num = %lu\n", writer - 1, num);
                    fprintf(stderr, "Trackers: ");
                    int k;
                    for (k = 0; k < numWriters; ++k) {
                        fprintf(stderr, "%lu\t", trackers[k]);
                    }
                    fprintf(stderr, "\n");
                    return;
                }
            }

            // If we get to this point, the test was successful!
            printf("Success!\n");
            return;
            break;
        }
    }

    // Writer
    unsigned long j;
    for (j = 0; j < longs_per_writer; ++j) {
        unsigned long num = j + (my_procnum << 24); // Encode the writer id
        //printf("%d\t", my_procnum << 24);
        fifo_wr(f, num);
    }
}
