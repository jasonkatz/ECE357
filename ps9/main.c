#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "sched.h"

void init();
void child();

void handle_int(int signum) {
    sched_ps();
    exit(1);
}

int numChildren = 4;

int main(int argc, char ** argv) {

    // On interrupt, print stats
    signal(SIGINT, &handle_int);

    sched_init(init);

    return 0;
}

void init() {
    int i;
    for (i = 0; i < numChildren; ++i) {
        int pid;
        switch(pid = sched_fork()) {
            case -1:
                fprintf(stderr, "Error forking process\n");
                exit(1);
                break;
            case 0:
                sched_nice(i * 4);
                child();
                break;
        };
    }

    int code;
    for (i = 0; i < numChildren; ++i) {
        sched_wait(&code);
        printf("Child with pid %d exited\n", code);
    }
    printf("done waiting\n");
    exit(0);
}

void child() {
    long long i;
    for (i = 0; i < 1000000000; ++i) {
        if (i % 100000000 == 0) {
            //printf("Child with pid %d at %ld ticks\n", sched_getpid(), sched_getcurrenttick());
        }
    }
    sched_exit(sched_getpid());
}
