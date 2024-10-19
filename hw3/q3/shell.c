#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "macros.h"
#include "proc_runner.h"
#include "tokenizer.h"

#define GETPWD_BUF_SIZE 256

#define USAGE(name) \
    fprintf(stdout, "Usage: %s {script}\n", name);

#define DISPLAY_PS1() \
    printf("%s:%s$ ", prompt, pwd); fflush(stdout);

double timeval2sec(struct timeval input) {
    return 
    ((double) input.tv_sec + ((double) input.tv_usec / (double) 1000000));
}

// TODO MOVE THIS TO ITS OWN FILE, and add a switch case to detect the type of
// fault, if its a signal.
void report_child_status(pid_t pid, int wstatus, struct rusage *ru, double real) {
    // report cause of death
    if(WIFEXITED(wstatus)) {
        printf("Child %ld exited with exit status %d",
                  pid, WEXITSTATUS(wstatus));
    }
    if(WIFSIGNALED(wstatus)) {
        printf("Child %ld terminated by signal %d",
                  pid, WTERMSIG(wstatus));
#ifdef WCOREDUMP
        if(WCOREDUMP(wstatus)){
            printf(" (core dumped)");
        }
#endif  
    }
    printf("\n");
    // report time lived
    double user = timeval2sec(ru->ru_utime);
    double sys = timeval2sec(ru->ru_stime);
    printf("Real: %.3fs User: %.3lfs Sys: %.3lfs\n", 
        real, user, sys);
    return;
}

int main(int argc, char *argv[]) 
{
    FILE *in;
    switch(argc) {
        case 1: // use stdin / interactive mode 
            in = stdin;
            break;
        case 2: // stdin redirection to script
            in = freopen(argv[1], "r", stdin);
            if(in == NULL) {
                ERR_CLOSE("Cannot open script %s for reading! %s", argv[1]);
            }
            break;
        default:
            USAGE(argv[0]);
            return EXIT_FAIL;
    }

    // State variables for execution loop
    struct rusage *ru = malloc(sizeof(struct rusage));
    
    char *line = NULL;
    size_t len = 0;
    ssize_t nread = 0;
    char *pwd;
    unsigned int last_exit = 0;
    if((pwd = getcwd(NULL, 0)) == NULL) {
        ERR_CONT("Unable to set cwd! %s\nDefaulting to '/'");
        pwd = "/"; // YOU SHOULD CD HERE!
    }

    while((nread = getline(&line, &len, in)) > 0) {
        if(line[0] == '#' || line[0] == '\n') { continue; }
        if(line[nread-1] == '\n') { line[nread-1] = '\0'; }
        
        ERR_CONT("line: \"%s\", bytes read: %ld", line, nread);
        tokenized_line *in_cmd = linetok(line);
        if(in_cmd == NULL) {
            ERR_CLOSE("%s: Failed to tokenize line \"%s\"! %s", 
                       argv[0], line);
        }
        // clean the buffer for our child, if we are reading in from a file
        fflush(in);

        pid_t pid;
        int wstatus;
        time_t t;
        switch(pid = fork()) {
            case -1:
                ERR_CONT("%s: Could not fork during creation of child for %s! %s",
                           argv[0], (in_cmd->argv)[0]);
                break;
            case 0:
                exit(proc_runner(in_cmd));
            default:
                if(wait3(&wstatus, 0, ru) < 0) {
                    ERR_CONT("%s: Error waiting for child, pid %d during %s! %s", 
                              argv[0], pid, (in_cmd->argv)[0]);
                }
                // set last_exit here
                report_child_status(pid, wstatus, ru, -1);
                break;
        }

        free(in_cmd);
    }
    
    return 0; 
}
