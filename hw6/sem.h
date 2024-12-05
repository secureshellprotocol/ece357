#ifndef ERJR_SEM_H
#define ERJR_SEM_H
#include <unistd.h>

#include "spinlock.h"

// max number of processes we can support.
#define N_PROC 64

struct sem {
    volatile char lock;
//    volatile char count_lock;
    int count;

//    volatile char wait_queue_lock;
    pid_t wait_queue[N_PROC];

    // Instrumentation Extenstion
    int wakeup_count[N_PROC];
    int sleep_count[N_PROC];
};


// Initializes an already allocated 'struct sem *s' with some count 'count'.
// Must only be called once per semaphore.
void sem_init(struct sem *s, int count);

// Attempt to decrement our semaphore by 1
// Returns
//  0 if we would have to block (our semaphore count is not positive)
//  1 otherwise.
int sem_try(struct sem *s);

// Block until we can decrement our semaphore by 1.
// Return once we are done waiting and have successfully decremented.
// A virtual processor id (vpid) must be specified! Each process sharing the 
//  semaphor should be given their own unique id, and it must be declared 
//  between 0 and N_PROC-1
void sem_wait(struct sem *s, int vpid);

// Increment our semaphore by 1. Sends SIGUSR1 to all blocked processes to wake
// them up.
void sem_inc(struct sem *s);

#endif
