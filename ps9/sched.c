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
    new_proc->total_ticks = parent->total_ticks;

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

void sched_exit(int code) {
    // Block all signals (critical region)
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    --proc_count;
    current->state = SCHED_ZOMBIE;
    current->exit_code = code;

    // Wake up a sleeping parent, if there is one
    int i;
    for (i = 0; i < SCHED_NPROC; ++i) {
        if (running->procs[i] && running->procs[i]->pid == current->ppid && running->procs[i]->state == SCHED_SLEEPING) {
            current = running->procs[i];
            current->state = SCHED_RUNNING;
            // Return the exit code to the parent
            restorectx(&(current->ctx), 1);
        }
    }

    // Unblock all signals (end of critical region)
    sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

    sched_switch();
}

int sched_wait(int * exit_code) {
    // Block all signals (critical region)
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    // Check for children
    int i;
    int found_child = 0;
    for (i = 0; i < SCHED_NPROC; ++i) {
        if (running->procs[i] && running->procs[i]->ppid == current->pid) {
            found_child = 1;
            break;
        }
    }
    if (!found_child) {
        fprintf(stderr, "No child to wait for\n");
        return -1;
    }

    // Wait until we get a zombie child
    struct sched_proc * zombie_child;
    for (;;) {
        // Check for zombie children
        int found_zombie = 0;
        for (i = 0; i < SCHED_NPROC; ++i) {
            if (running->procs[i] && running->procs[i]->ppid == current->pid && running->procs[i]->state == SCHED_ZOMBIE) {
                found_zombie = 1;
                zombie_child = running->procs[i];
                break;
            }
        }
        if (found_zombie) {
            // Get rid of zombie process memory
            free(running->procs[i]);
            running->procs[i] = NULL;

            // Stop waiting if zombie found
            break;
        }

        // Sleep if no zombie
        current->state = SCHED_SLEEPING;
        
        // Unblock signals, switch processes, then reblock signals
        sigprocmask(SIG_UNBLOCK, &block_mask, NULL);
        sched_switch();
        sigprocmask(SIG_BLOCK, &block_mask, NULL);
    }

    *exit_code = zombie_child->exit_code;

    // Unblock all signals (end of critical region)
    sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

    return 0;
}

void sched_nice(int niceval) {
    if (niceval > 19) {
        niceval = 19;
    } else if (niceval < -20) {
        niceval = -20;
    }

    current->niceval = niceval;
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

long sched_getcurrenttick() {
    return current->total_ticks;
}

void sched_ps() {
    fprintf(stdout, "pid\tppid\tcurrent state\tstack addr\tniceval\tpriority\ttotal CPU time (ticks)\n");

    int i;
    for (i = 0; i < SCHED_NPROC; ++i) {
        if (!running->procs[i]) {
            continue;
        }
        struct sched_proc * proc = running->procs[i];
        fprintf(stdout, "%d\t%d\t%d\t\t%p\t%d\t%d\t\t%ld\n", proc->pid, proc->ppid, proc->state, proc->stack, proc->niceval, proc->priority, proc->total_ticks);
    }
}

void sched_switch() {
    // Block all signals (critical region)
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    // Update all priorities
    int i;
    for (i = 0; i < SCHED_NPROC; ++i) {
        if (running->procs[i]) {
            int temp = 20 - running->procs[i]->niceval - (running->procs[i]->total_ticks / (20 - running->procs[i]->niceval));
            running->procs[i]->priority = temp;
        }
    }

    // Update the current state
    if (current->state != SCHED_SLEEPING && current->state != SCHED_ZOMBIE) {
        current->state = SCHED_READY;
    }

    // Select the READY task with the highest priority
    int highest_priority = 0, highest_priority_index = -1;
    for (i = 0; i < SCHED_NPROC; ++i) {
        // Only look at processes in the READY state
        if (running->procs[i] && running->procs[i]->state == SCHED_READY) {
            // Select process with the highest priority
            if (running->procs[i]->priority > highest_priority) {
                highest_priority = running->procs[i]->priority;
                highest_priority_index = i;
            }
        }
    }
    if (highest_priority_index == -1) {
        //fprintf(stderr, "No ready processes on wait queue\n");
        return;
    }

    // Check for case where the current process is also the highest priority
    if (running->procs[highest_priority_index]->pid == current->pid) {
        //fprintf(stderr, "Highest priority process is also current; nothing changes\n");
        fprintf(stderr, "Remaining in process %d\n", current->pid);
        current->state = SCHED_READY;
        return;
    }

    struct sched_proc * best_proc = running->procs[highest_priority_index];
    fprintf(stdout, "Switching to pid %d\n", best_proc->pid);
    sched_ps();

    // Current process goes on the wait queue
    // and context is switched to new process
    if (!savectx(&(current->ctx))) {
        best_proc->state = SCHED_RUNNING;
        current = best_proc;
        restorectx(&(current->ctx), 1);
    }

    // Unblock all signals (end of critical region)
    sigprocmask(SIG_UNBLOCK, &block_mask, NULL);
}

void sched_tick() {
    ++(current->total_ticks);
    ++tick_count;
    sched_switch();
}
