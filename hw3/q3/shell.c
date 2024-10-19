#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "macros.h"
#include "proc_runner.h"
#include "tokenizer.h"

#define GETPWD_BUF_SIZE 256

#define USAGE(name) \
    fprintf(stdout, "Usage: %s {script}\n", name);

#define DISPLAY_PS1() \
    printf("%s:%s$ ", prompt, pwd); fflush(stdout);

double timeval2milli(struct timeval input) {
    return 
    ((double) input.tv_sec + ((double) input.tv_usec / (double) 1000000)) * 1000;
}

void report_child_status(pid_t pid, int wstatus, struct rusage *ru) {
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
    printf("user time spent: %.3lf millis\n", timeval2milli(ru->ru_utime));
    printf("system time spent: %.3lf millis\n", timeval2milli(ru->ru_stime));

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
            in = fopen(argv[1], "r");
            if(in == NULL) {
                ERR_CLOSE("Cannot open script %s for reading! %s", argv[1]);
            }
            break;
        default:
            USAGE(argv[0]);
            return EXIT_FAIL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread = 0;
    
    char *pwd;
    if((pwd = getcwd(NULL, 0)) == NULL) {
        ERR_CONT("Unable to set cwd! %s\nDefaulting to '/'");
        pwd = "/";
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

        pid_t pid;
        int wstatus;
        struct rusage *ru = malloc(sizeof(struct rusage));
        switch(pid = fork()) {
            case -1:
                ERR_CLOSE("%s: Could not fork during creation of child for %s! %s",
                           argv[0], (in_cmd->argv)[0]);
            case 0:
                exit(proc_runner(in_cmd));
            default:
                if(wait3(&wstatus, 0, ru) < 0) {
                    ERR_CONT("%s: Error waiting for child, pid %d during %s! %s", 
                              argv[0], pid, (in_cmd->argv)[0]);
                }
                report_child_status(pid, wstatus, ru);
                break;
        }
        free(in_cmd);
    }
    free(line); 
    return 0; 
}
