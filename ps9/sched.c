#include <stdio.h>
#include <errno.h>

#include "sched.h"

struct sched_proc * current;

int sched_init(void (*init_fn)()) {
}

int sched_fork() {
    // Create new stack area
    void * newsp;
    if ((newsp = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) < 0) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    }
}

int sched_exit(int code) {
}

int sched_wait(int * exit_code) {
}

int sched_nice(int niceval) {
}

int sched_getpid() {
}

int sched_getppid() {
}

// MAY HAVE TO MAKE THIS LONG?
int sched_gettick() {
}

void sched_ps() {
}

void sched_switch() {
}

void sched_tick() {
}
