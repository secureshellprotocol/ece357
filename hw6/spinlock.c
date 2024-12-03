#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spinlock.h"
#include "tas.h"

// spin_lock() - try to gain control of our lock.
// Returns
//  0 if lock was acquired
//  non-zero upon failure
int spin_lock(volatile char *lock) 
{
    while(tas(lock)!=0) {
        if(sched_yield() < 0) {
            fprintf(stderr, "sched_yield: failed to yield: ");
            perror(strerror(errno));
            return EXIT_FAILURE;
       }
    }
    return EXIT_SUCCESS;
}

// spin_unlock() - unlock our lock
// Returns when lock is freed from our control.
void spin_unlock(volatile char *lock) 
{
    *lock = 0;
    return;
}

