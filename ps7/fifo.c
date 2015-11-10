#include "fifo.h"

void fifo_init(struct fifo * f) {
    sem_init(&(f->access_lock), 1); // Access lock should start with 1 resource (the lock)
    sem_init(&(f->rd_sem), 0); // Read semaphore should start with no resources
    sem_init(&(f->wr_sem), MYFIFO_BUFSIZ); // Write semaphore should start with all resources
    f->front = 0;
    f->end = 0;
    f->count = 0;
}

void fifo_wr(struct fifo * f, unsigned long d) {
    sem_wait(&(f->access_lock)); // Wait for available access
    sem_wait(&(f->wr_sem)); // Wait for available write resource

    // Insert new element at front of fifo
    f->buffer[f->front++] = d;
    f->front %= MYFIFO_BUFSIZ;
    f->count++;

    sem_inc(&(f->rd_sem)); // Indicate one more available resource for read
    sem_inc(&(f->access_lock)); // Unlock access
}

unsigned long fifo_rd(struct fifo * f) {
    sem_wait(&(f->access_lock)); // Wait for available access
    sem_wait(&(f->rd_sem)); // Wait for available read resource

    // Remove element from end of fifo
    unsigned long val = f->buffer[f->end++];
    f->end %= MYFIFO_BUFSIZ;
    f->count--;

    sem_inc(&(f->wr_sem)); // Indicate one more availabe resource for write
    sem_inc(&(f->access_lock)); // Unlock access

    return val;
}
