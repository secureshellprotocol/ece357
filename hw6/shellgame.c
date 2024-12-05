#define _GNU_SOURCE
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "macros.h"
#include "sem.h"

#define USAGE(name) \
    printf("Usage: %s initial_count iterations\n", name); return EXIT_FAIL;

int my_procnum;
int sigusr1_invocation_count;

void sigusr1_handler(int s)
{
    sigusr1_invocation_count++;
}

int getfrom(int vpid)
{
    switch(vpid)
    {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 0;
        case 3:
            return 2;
        case 4:
            return 1;
        case 5:
            return 2;
    }
}

int getto(int vpid)
{
    switch(vpid)
    {
        case 0:
            return 1;
        case 1:
            return 0;
        case 2:
            return 2;
        case 3:
            return 0;
        case 4:
            return 2;
        case 5:
            return 1;
    }
}

// Converts strings to integers using strtol. Conversions to negative integers
// are thrown out and we return EXIT_FAIL
int arg_convert(char *arg)
{
    long argnum = strtol(arg, NULL, 10);
    if(argnum <= INT_MAX && argnum > 0)
    {
        return (int) argnum;
    }
    return EXIT_FAIL;
}

// reports status of child to stdout
void report_child_status(pid_t pid, int wstatus)
{
    if(WIFEXITED(wstatus))
    {
        ERR("Child w/ pid %d exited w/ %d\n", pid, WEXITSTATUS(wstatus));
    }
    else if(WIFSIGNALED(wstatus))
    {
        ERR("Child w/ pid %d terminated by signal %d", pid, WTERMSIG(wstatus));
#ifdef WCOREDUMP
        if(WCOREDUMP(wstatus))
        {
            ERR(" (core dumped)");
        }
#endif
        ERR("\n");
    }
    return;
}

struct sem *generate_semlist(int count)
{
    struct sem *semlist = 
        (struct sem*) mmap(NULL, 3 * sizeof(struct sem), PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(semlist == MAP_FAILED)
    {
        ERR_EXT("generate_semlist: failed to get mappings! %s");
        return NULL;
    }

    sem_init(&(semlist[0]), count);
    sem_init(&(semlist[1]), count);
    sem_init(&(semlist[2]), count);

    return semlist;
}

int establish_sigusr1_override()
{ 
    sigusr1_invocation_count = 0;
    
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = 0;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGUSR1);
    if(sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        ERR_EXT("establish_sigusr1_override: sigaction: %s\n");
        return EXIT_FAIL;
    }
    return EXIT_SUCCESS;
}

void init_player(struct sem *from, struct sem *dest, int iterations)
{
    pid_t pid = getpid();
    ERR("VCPU %d starting, pid %d\n", my_procnum, pid);

    for(int turn = 0; turn < iterations; turn++)
    {
        sem_wait(from, my_procnum);
        sem_inc(dest);
    }

    ERR("Child %d (pid %d) done, signal handler was invoked %d times\n",
        my_procnum, pid, sigusr1_invocation_count);
    
    exit(EXIT_SUCCESS);
}

void report_shell_stats(struct sem *shells)
{
    printf("Sem#\tval\tSleeps\tWakes\n");
    for(int shell = 0; shell < 3; shell++)
    {
        printf("%d\t%d\n", shell, shells[shell].count);
        for(int vcpu = 0; vcpu < 6; vcpu++){
            printf(" VCPU%d\t\t%d\t%d\n", vcpu, 
                    shells[shell].sleep_count[vcpu],
                    shells[shell].wakeup_count[vcpu]);
        }
    }
}

int start_shellgame(int count, int iterations)
{
    struct sem *from, *destination;
    struct sem *shells = generate_semlist(count);

    if(shells == NULL) { return EXIT_FAIL; }

    // set up SIGUSR1 handler
    if(establish_sigusr1_override() < 0)
    {
        ERR("shellgame: Failed to set override for sigusr1!\n");
        return EXIT_FAIL;
    }

    pid_t fork_pid;
    for(my_procnum = 0; my_procnum < 6; my_procnum++)
    {
        from = &(shells[getfrom(my_procnum)]);
        destination = &(shells[getto(my_procnum)]);

        switch(fork_pid = fork())
        {
            case -1:
                return EXIT_FAIL;
            case 0:
                init_player(from, destination, iterations);
                // never returns
            default:
                break;
        }
    }

    // Parent process waits
    ERR("Main process spawned all children, waiting\n");

    int wstatus;
    while((fork_pid = wait(&wstatus)) > 0)
    {
        report_child_status(fork_pid, wstatus);
    }

    ERR("All children done!\n");
    report_shell_stats(shells);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if(argc != 3) { USAGE(argv[0]); } // --> EXIT_FAIL

    int count, iterations;
    count = arg_convert(argv[1]);
    if(count == EXIT_FAIL)
    {
        ERR("%s: Invalid initial count provided!\n", argv[0]);
        return EXIT_FAIL;
    }
    iterations = arg_convert(argv[2]);
    if(iterations == EXIT_FAIL)
    {
        ERR("%s: Invalid iterations provided!\n", argv[0]);
        return EXIT_FAIL;
    }

    return start_shellgame(count, iterations);
}
