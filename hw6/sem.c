#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sem.h"
#include "spinlock.h"

// max number of processes we can support.
#define N_PROC 64
struct sem;

// Initializes an already allocated 'struct sem *s' with some count 'count'.
// Must only be called once per semaphore.
void sem_init(struct sem *s, int count)
{
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
    spin_lock(&(s->count_lock));
    if(s->count > 0)
    {
        s->count = s->count - 1;
        spin_unlock(&(s->count_lock));
        return 1;
    }

    spin_unlock(&(s->count_lock));
    return 0;
}

// Block until we can decrement our semaphore by 1.
// Return once we are done waiting and have successfully decremented.
// A virtual processor id (vpid) must be specified! Each process sharing the 
//  semaphor should be given their own unique id, and it must be declared 
//  between 0 and N_PROC-1
void sem_wait(struct sem *s, int vpid)
{
    sigset_t set, oldmask;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    
    pid_t pid = getpid();

    while(1)
    {
        if(sem_try(s) == 1) { return; }
        
        // Could not decrement! We enter a wait queue, blocking until we receive 
        // SIGUSR1. We'll try again once we get that signal.
        sigprocmask(SIG_BLOCK, &set, &oldmask); // SIGUSR1 masked
        spin_lock(&(s->wait_queue_lock));
        
        s->wait_queue[vpid] = pid;
        (s->sleep_count[vpid])++;
        
        spin_unlock(&(s->wait_queue_lock));
        sigsuspend(&oldmask); // SIGUSR1 unmasked
        sigprocmask(SIG_UNBLOCK, &set, NULL);
    }
}

// Increment our semaphore by 1. Sends SIGUSR1 to all blocked processes to wake
// them up.
void sem_inc(struct sem *s)
{
    spin_lock(&(s->count_lock));
    s->count = s->count + 1;
    
    if(s->count > 0)
    {
        spin_lock(&(s->wait_queue_lock));
        for(int vpid = 0; vpid < N_PROC; vpid++)
        {

            if(s->wait_queue[vpid] != 0 && kill(s->wait_queue[vpid], SIGUSR1)<0) 
            {
                fprintf(stderr, "sem_inc: failed to send SIGUSR1 to pid %d: %s",
                        s->wait_queue[vpid], strerror(errno));
            }
            s->wait_queue[vpid] = 0;

        }
        spin_unlock(&(s->wait_queue_lock));
    }

    spin_unlock(&(s->count_lock));
}