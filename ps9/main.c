#include <stdio.h>
#include <stdlib.h>

#include "sched.h"

void func();

int main(int argc, char ** argv) {

    sched_init(func);

    return 0;
}

void func() {
    int pid;
    switch(pid = sched_fork()) {
        case -1:
            fprintf(stderr, "Error forking process\n");
            exit(1);
            break;
        case 0:
            printf("in child\n");
            sched_exit(2);
            break;
        default:
            printf("in parent\n");
            break;
    };

    int code;
    sched_wait(&code);
    exit(0);
}
