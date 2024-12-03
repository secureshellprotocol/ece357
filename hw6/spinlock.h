#ifndef ERJR_SPINLOCK_H
#define ERJR_SPINLOCK_H

// Small Spinlock library

// spin_lock() - try to gain control of our lock.
// Returns
//  0 if lock was acquired
//  non-zero upon failure
int spin_lock(volatile char *lock);

// spin_unlock() - unlock our lock
// Returns when lock is freed from our control.
void spin_unlock(volatile char *lock);

#endif
