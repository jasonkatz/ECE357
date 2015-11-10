#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "sem.h"

int tas(volatile char * lock);

void sem_init(struct sem * s, int count) {
    s->count = count;
    s->lock = 0;

    // Set procs array to zeroes
    int i;
    for (i = 0; i < N_PROC; ++i) {
        s->procs[i] = 0;
    }
}

int sem_try(struct sem * s) {
    while (tas(&s->lock))
        ;

    if (s->count) {
        s->count--;
    }

    s->lock = 0;

    return (s->count + 1 > 0);
}

void usr1_handler(int signum) { }

void sem_wait(struct sem * s) {
    s->procs[my_procnum] = getpid(); // Set procs flag to indicate waiting state

    for (;;) {
        while (tas(&(s->lock)))
            ;

        // Block all signals but SIGUSR1 (handle SIGUSR1)
        signal(SIGUSR1, usr1_handler);
        sigset_t old_mask, new_mask;
        sigfillset(&new_mask);
        sigdelset(&new_mask, SIGINT);
        sigdelset(&new_mask, SIGUSR1);
        sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

        if (s->count > 0) {
            s->procs[my_procnum] = 0;
            s->count--;
            s->lock = 0;
            return;
        }

        s->lock = 0;
        sigsuspend(&new_mask);
        sigprocmask(SIG_UNBLOCK, &new_mask, NULL);
    }
}

void sem_inc(struct sem * s) {
    while (tas(&s->lock))
        ;

    s->count++;

    // Wake all sleeping processes with SIGUSR1
    int i;
    for (i = 0; i < N_PROC; ++i) {
        if (s->procs[i]) {
            kill(s->procs[i], SIGUSR1);
        }
    }

    s->lock = 0;
}
