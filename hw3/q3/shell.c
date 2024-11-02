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
#include "shell.h"
#include "tokenizer.h"
#include "utils.h"

// converts timeval to seconds, as a double.
double timeval2sec(struct timeval input) {
    return 
        ((double) input.tv_sec + ((double) input.tv_usec / (double) 1000000));
}

// reports the status of our child to stdout.
// returns the exit code if the child exited successfully, or returns the 
//  signal number if the child was killed by a signal.
int report_child_status(pid_t pid, unsigned int wstatus, struct rusage *ru, double real) 
{
    int status = 0;

    // report cause of death
    if(WIFEXITED(wstatus)) {
        status = WEXITSTATUS(wstatus);
        ERR_CONT("Child %ld exited with exit status %d",
                pid, status);
    }
    if(WIFSIGNALED(wstatus)) {
        status = WTERMSIG(wstatus);
        ERR_CONT_NONL("Child %ld terminated by signal %d",
                     pid, status);
#ifdef WCOREDUMP
        if(WCOREDUMP(wstatus)){
            ERR_CONT_NONL(" (core dumped)");
            status += 128;
        }
#endif  
        ERR_CONT();
    }

    // report time lived
    double user = timeval2sec(ru->ru_utime);
    double sys = timeval2sec(ru->ru_stime);
    printf("Real: %.3fs User: %.3lfs Sys: %.3lfs\n", 
        real, user, sys);
    
    return status;
}

int main(int argc, char *argv[]) 
{
    FILE *in;
    switch(argc) {
        case 1: // use stdin / interactive mode 
            in = stdin;
            break;
        case 2: // stdin redirection to script
            in = fopen(argv[1], "r");
            if(in == NULL) {
                ERR_CLOSE("%s: Cannot open script %s for reading! %s", 
                          argv[0], argv[1]);
            }
            break;
        default: // print usage
            USAGE(argv[0]);
            return EXIT_FAIL;
    }

    // State variables for execution loop
    struct rusage *ru = malloc(sizeof(struct rusage));  // stats collected by
                                                        // wait - reused during
                                                        // each loop
    
    char *line = NULL;          // Line read in by getline()
    size_t len = 0;             //      associated length of `line`
    ssize_t nread = 0;          //      no of bytes read in from input
    unsigned int last_exit = 0; // Last status code returned to us.
    
    while((nread = getline(&line, &len, in)) > 0) {
        if(line[0] == '#' || line[0] == '\n') { continue; }
        if(line[nread-1] == '\n') { line[nread-1] = '\0'; }

        tokenized_line *in_cmd = linetok(line);
        if(in_cmd == NULL) {
            ERR_CONT("%s: linetok: Unrecoverable failure! failed to tokenize line \"%s\"! %s", 
                     argv[0], line);
            continue;
        }
        // clean the buffer for our child, if we are reading in from a file
        fflush(in);
        
        // parse special commands (`cd`, `pwd`, `exit`)
        if(strcmp(in_cmd->argv[0], "cd") == 0) {
            last_exit = cd(in_cmd->argc, in_cmd->argv);
            continue;
        }

        if(strcmp(in_cmd->argv[0], "pwd") == 0) {
            pwd();
            last_exit = 0;
            continue;
        }
        
        if(strcmp(in_cmd->argv[0], "exit") == 0) {
            int code = last_exit;
            if(in_cmd->argc > 1) {
                if((code = strtol(in_cmd->argv[1], NULL, 10)) == 0) {
                    ERR_CONT_NONL("%s: Could not parse error code %s", 
                                 argv[0], in_cmd->argv[1]);
                }
            }
            ERR_CONT("%s exited with code %d", argv[0], code);
            exit(code);
        }
    
        pid_t pid;                  // PID of forked child
        unsigned int wstatus;       // status of waited-on child
        struct timeval start, stop; // time spent waiting

        gettimeofday(&start, NULL);
        switch(pid = fork()) {
            case -1: // fork failed
                ERR_CONT("%s: Could not fork during creation of child for %s! %s",
                        argv[0], (in_cmd->argv)[0]);
                break;
            case 0: // child routine
                exit(proc_runner(in_cmd));
            default: // parent routine
                if(wait3(&wstatus, 0, ru) < 0) {
                    ERR_CONT("%s: Error waiting for child, pid %d during %s! %s", 
                            argv[0], pid, (in_cmd->argv)[0]);
                }

                gettimeofday(&stop, NULL);
                double rtime = timeval2sec(stop) - timeval2sec(start);
                last_exit = report_child_status(pid, wstatus, ru, rtime);
                break;
        }

        free(in_cmd); 
    }
    
    ERR_CONT("%s exited with code %d", argv[0], last_exit);
    return last_exit; 
}
