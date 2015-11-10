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
            for (i = 0; i < 10000; ++i) {
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
    for (i = 0; i < 10000; ++i) {
        success &= (i == fifo_rd(f));
    }
    printf("Result: %d\n", success);
}

void test2() {
}
