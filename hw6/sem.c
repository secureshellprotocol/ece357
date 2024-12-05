#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "macros.h"
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
    if(spin_lock(&(s->lock)) == EXIT_FAIL) 
    {
        ERR_EXT("sem_try: failed to yield for count lock: %s\n");
        return 0;
    }
    switch(s->count > 0)
    {
        case 1:
            s->count = s->count - 1;
            spin_unlock(&(s->lock));
            return 1;
        default:
            spin_unlock(&(s->lock));
            return 0;
    }
}

// Block until we can decrement our semaphore by 1.
// Return once we are done waiting and have successfully decremented.
// A virtual processor id (vpid) must be specified! Each process sharing the 
//  semaphor should be given their own unique id, and it must be declared 
//  between 0 and N_PROC-1
void sem_wait(struct sem *s, int vpid)
{
    sigset_t set, oldset;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
   
    pid_t pid = getpid();

    spin_lock(&(s->lock));
    while(1)
    {
        if(s->count > 0) {
			s->count = s->count - 1;
			goto unlock;
		}
		// Could not decrement! We enter a wait queue, blocking until we receive 
        // SIGUSR1. We'll try again once we get that signal.

        if(sigprocmask(SIG_BLOCK, &set, &oldset) < 0)
        {
            ERR_EXT("sem_wait: sigprocmask: failed to block SIGUSR1: %s\n");
            continue;
        }

        (s->sleep_count[vpid])++;
        s->wait_queue[vpid] = pid;
        spin_unlock(&(s->lock));
        
        sigsuspend(&oldset);
        
        spin_lock(&(s->lock));
        s->wait_queue[vpid] = 0;
        
        if(sigprocmask(SIG_UNBLOCK, &set, NULL) < 0)
        {
            ERR_EXT("sem_wait: sigprocmask: failed to unblock SIGUSR1: %s\n");
        }
    }

unlock:
	spin_unlock(&(s->lock));
    return;
}

// Increment our semaphore by 1. Sends SIGUSR1 to all blocked processes to wake
// them up.
void sem_inc(struct sem *s)
{
    // Spinlock -- wait for our turn to access count, so we can increment and 
    // check if its now a positive value
    if(spin_lock(&(s->lock)) == EXIT_FAIL)
    {
        ERR_EXT("sem_inc: failed to acquire lock for count: %s\n");
        return;
    }
    s->count = s->count + 1;
    
    if(s->count > 0)
    {
        // wake up every waiting process with SIGUSR1
        for(int vpid = 0; vpid < N_PROC; vpid++)
        {
            if(s->wait_queue[vpid] != 0)
            {
                if(kill(s->wait_queue[vpid], SIGUSR1)<0)
                {
                    ERR_EXT("sem_inc: failed to send SIGUSR1 to pid %d: %s\n",
                            s->wait_queue[vpid]);
                }
                (s->wakeup_count[vpid])++;
            }
        }
    }
    
    spin_unlock(&(s->lock));
    return;
}
