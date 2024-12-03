#ifndef ERJR_SEM_H
#define ERJR_SEM_H
#define _GNU_SOURCE
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include "spinlock.h"

// max number of processes we can support.
#define N_PROC 64

struct sem {
    volatile char *count_lock;
    int count;

    volatile char *wait_queue_lock;
    pid_t wait_queue[N_PROC];
};


// Initializes an already allocated 'struct sem *s' with some count 'count'.
// Must only be called once per semaphore.
void sem_init(struct sem *s, int count)
{
    // generate lock for count and queue
    s->count_lock = (char *) mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, 
                                  MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    s->wait_queue_lock = (char *) mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, 
                                       MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    // set count
    s->count = count;

    return;
}

// Attempt to decrement our semaphore by 1
// Returns
//  0 if we would have to block (our semaphore count is not positive)
//  1 otherwise.
int sem_try(struct sem *s)
{
    if(s->count > 0)
    {
        spin_lock(s->count_lock);
        if(s->count > 0)
        {
            s->count = s->count - 1;
            spin_unlock(s->count_lock);
            
            return 1;
        }
        spin_unlock(s->count_lock);
    }

    return 0;
}

// Block until we can decrement our semaphore by 1.
// Return once we are done waiting and have successfully decremented.
// A virtual processor id (vpid) must be specified! Each process sharing the 
//  semaphor should be given their own unique id, and it must be declared 
//  between 0 and N_PROC-1
void sem_wait(struct sem *s, int vpid)
{
    sigset_t block_sigusr1, oldmask;
    sigemptyset(&block_sigusr1);
    sigaddset(&block_sigusr1, SIGUSR1);
    
    pid_t pid = getpid();

    while(1)
    {
        if(sem_try(s) == 1) { break; }
        // Could not decrement! We enter a wait queue, blocking until we receive 
        // SIGUSR1. We'll try again once we get that signal.
        // add ourselves to wait queue
        spin_lock(s->wait_queue_lock);
        sigprocmask(SIG_BLOCK, &block_sigusr1, &oldmask);
        
        s->wait_queue[vpid] = pid;        

        spin_unlock(s->wait_queue_lock);
        sigsuspend(&oldmask);
        sigprocmask(SIG_UNBLOCK, &block_sigusr1, NULL);
    }
    return;
}

// Increment our semaphore by 1. Sends SIGUSR1 to all blocked processes to wake
// them up.
void sem_inc(struct sem *s)
{
    spin_lock(s->count_lock);
    s->count = s->count + 1;
    spin_unlock(s->count_lock);
    
    if(s->count > 0)
    {

    }
}

#endif
