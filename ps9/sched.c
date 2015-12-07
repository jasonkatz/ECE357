#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "sched.h"
#include "jmpbuf-offsets64.h"

struct sched_proc * current;
struct sched_waitq * running;
int proc_count;
long tick_count;
sigset_t block_mask;

int nextpid() {
    int i;
    for (i = 1; i <= SCHED_NPROC; ++i) {
        if (!running->procs[i - 1]) {
            return i;
        }
    }
    return -1;
}

int sched_init(void (*init_fn)()) {
    tick_count = 0;

    // Allocate running processes queue
    if (!(running = (struct sched_waitq *)malloc(sizeof(struct sched_waitq)))) {
        fprintf(stderr, "Error allocating wait queue\n");
        exit(1);
    }

    // Initialize PIT to trigger every 100ms
    struct itimerval timer_val;
    timer_val.it_value.tv_sec = 0;
    timer_val.it_value.tv_usec = 100000;
    timer_val.it_interval.tv_sec = 0;
    timer_val.it_interval.tv_usec = 100000;
    if (setitimer(ITIMER_VIRTUAL, &timer_val, NULL) < 0) {
        fprintf(stderr, "Error setting timer: %s\n", strerror(errno));
        exit(1);
    }

    // Establish sched_tick as the signal handler for PIT
    signal(SIGVTALRM, sched_tick);

    // Set up signals block mask
    sigfillset(&block_mask);

    // Create init process
    struct sched_proc * p;
    if (!(p = (struct sched_proc *)malloc(sizeof(struct sched_proc)))) {
        fprintf(stderr, "Error allocating initial process\n");
        exit(1);
    }

    // This should set the pid to 1
    if ((p->pid = nextpid()) < 0) {
        fprintf(stderr, "Couldn't get next pid\n");
        exit(1);
    }
    p->ppid = p->pid;
    if ((p->stack = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) < 0) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        exit(1);
    }
    p->state = SCHED_RUNNING;
    p->priority = 20;
    p->niceval = 0;
    p->total_ticks = 0;

    // Set globals
    current = p;
    running->procs[0] = p;
    proc_count = 1;

    // Transfer execution to init process
    struct savectx ctx;
    ctx.regs[JB_SP] = STACK_SIZE + p->stack;
    ctx.regs[JB_BP] = STACK_SIZE + p->stack;
    ctx.regs[JB_PC] = init_fn;
    restorectx(&ctx, 0);
    current->ctx = ctx;
}

int sched_fork() {
    // Block all signals (critical region)
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    // Create new process
    struct sched_proc * new_proc, * parent;
    parent = current;
    if (!(new_proc = (struct sched_proc *)malloc(sizeof(struct sched_proc)))) {
        fprintf(stderr, "Error allocating forked process\n");
        return -1;
    }

    // Set new process attributes
    if ((new_proc->pid = nextpid()) < 0) {
        free(new_proc);
        fprintf(stderr, "Couldn't get new pid\n");
        return -1;
    }
    new_proc->ppid = parent->pid;
    new_proc->state = SCHED_READY;
    new_proc->priority = 20;
    new_proc->niceval = 0;
    new_proc->total_ticks = 0;

    // Create new stack area
    void * newsp;
    if ((newsp = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) < 0) {
        free(new_proc);
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        return -1;
    }

    // Copy current stack to new stack
    memcpy(newsp, current->stack, STACK_SIZE);
    new_proc->stack = newsp;

    // Adjust stack
    unsigned long stack_diff = new_proc->stack - parent->stack;
    adjstack(new_proc->stack, new_proc->stack + STACK_SIZE, stack_diff);

    // Adjust globals
    running->procs[new_proc->pid - 1] = new_proc;
    ++proc_count;

    int returnval;

    // savectx returns nonzero in child
    if (savectx(&new_proc->ctx)) {
        returnval = 0;
    } else {
        returnval = new_proc->pid;
        new_proc->ctx.regs[JB_SP] += stack_diff;
        new_proc->ctx.regs[JB_BP] += stack_diff;
    }

    // Unblock all signals (end of critical region)
    sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

    return returnval;
}

int sched_exit(int code) {
}

int sched_wait(int * exit_code) {
}

int sched_nice(int niceval) {
}

int sched_getpid() {
    return current->pid;
}

int sched_getppid() {
    return current->ppid;
}

long sched_gettick() {
    return tick_count;
}

void sched_ps() {
}

void sched_switch() {
}

void sched_tick() {
    ++(current->total_ticks);
    ++tick_count;
    printf("tick %ld\n", tick_count);
}
