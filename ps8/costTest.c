#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

void partA();
void partB();
void emptyFunction();
void partC();

int main(int argc, char ** argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: ./a.out partNum (1-3)\n");
        exit(1);
    }

    switch (atoi(argv[1])) {
        case 1:
            partA();
            break;
        case 2:
            partB();
            break;
        case 3:
            partC();
            break;
        default:
            fprintf(stderr, "Please enter 1, 2 or 3\n");
            break;
    }

    return 0;
}

void partA() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int i;
    for (i = 0; i < 100000; ++i) { }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time: %d nanoseconds\n", end.tv_nsec - start.tv_nsec);
}

void partB() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int i;
    for (i = 0; i < 100000; ++i) {
        emptyFunction();
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time: %d nanoseconds\n", end.tv_nsec - start.tv_nsec);
}

void emptyFunction() { }

void partC() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int i;
    for (i = 0; i < 100000; ++i) {
        getuid();
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time: %d nanoseconds\n", end.tv_nsec - start.tv_nsec);
}
