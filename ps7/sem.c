#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "sem.h"

int tas(volatile char * lock);

void sem_init(struct sem * s, int count) {
    s->count = count;
    s->lock = 0;

    s->procs = (int *)malloc(N_PROC * sizeof(int *));
}

int sem_try(struct sem * s) {
    return !(tas(&s->lock) && s->count > 0);
}

void sem_wait(struct sem * s) {
    // Block all signals except SIGUSR1 if in waiting state
    sigset_t old_mask, new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);
    while (!sem_try(s)) {
        // Set the corresponding pid in procs
        s->procs[my_procnum] = getpid();
        sigsuspend(&old_mask);
    }
    s->procs[my_procnum] = 0; // Don't forget to reset the procs flag
    sigprocmask(SIG_UNBLOCK, &old_mask, NULL);

    // Decrement
    s->count--;

    s->lock = 0;
}

void sem_inc(struct sem * s) {
    s->count++;

    // Wake all sleeping processes with SIGUSR1
    int i;
    for (i = 0; i < N_PROC; ++i) {
        if (s->procs[i]) {
            kill(s->procs[i], SIGUSR1);
            s->procs[i] = 0;
        }
    }
}
