#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "spinlock.h"

#define TEN 4000000

volatile char *lock;
int my_procnum;

void count_to_ten(long long *shared_number)
{
    for(int i = 0; i < TEN; i++) {
        (*shared_number)++;
    }
}

void locking_count_to_ten(long long *shared_number)
{
    for(int i = 0; i < TEN; i++) {
        spin_lock(lock);
        (*shared_number)++;
        spin_unlock(lock);
    }
}

int main(int argc, char *argv[]) 
{
    lock = (char*) mmap(NULL, 1 * sizeof(char), PROT_READ | PROT_WRITE, 
                        MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    long long *shared_number = 
        (long long *) mmap(NULL, sizeof(long long), PROT_READ | PROT_WRITE, 
                           MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *shared_number = 0;

    pid_t pid;
    // My iMac15,1 has an Intel i5-4590, with 4 cores. 
    for(my_procnum = 0; my_procnum < 4; my_procnum++)
    {
        pid = fork();
        if(pid == 0) 
        {
            fprintf(stderr, "\"I expected this reception,\" said daemon %d\n", 
                    my_procnum);
            break;
        }
    }
    if(pid == 0) 
    {
        count_to_ten(shared_number);
        return EXIT_SUCCESS;
    }

    pid_t child;
    int wstatus = 0;
    while((child = wait(&wstatus)) > 0);
    if(child < 0 && errno != ECHILD)
    {
        fprintf(stderr, "wait: failed to wait for child: ");
        perror(strerror(errno));
    }

    printf("Frankenstein says: My daemons counted to %lld!\n", *shared_number);

    // lock them in the cellar
    *shared_number = 0;

    for(my_procnum = 0; my_procnum < 4; my_procnum++)
    {
        pid = fork();
        if(pid == 0) 
        {
            fprintf(stderr, "\"I expected this reception,\" said daemon %d\n", 
                    my_procnum);
            break;
        }
    }
    if(pid == 0) 
    {
        locking_count_to_ten(shared_number);
        return EXIT_SUCCESS;
    }

    wstatus = 0;
    while((child = wait(&wstatus)) > 0);
    if(child < 0 && errno != ECHILD)
    {
        fprintf(stderr, "wait: failed to wait for child: ");
        perror(strerror(errno));
    }

    printf("Frankenstein says: My locked daemons counted to %lld!\n", *shared_number);
    return EXIT_SUCCESS;
}
