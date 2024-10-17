#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "macros.h"
#include "proc_runner.h"
#include "tokenizer.h"

#define READ_BUF 256

#define USAGE(name) \
    fprintf(stdout, "Usage: %s [script]\n", name);

int main(int argc, char *argv[]) 
{
    switch(argc) {
        case 1: // use stdin / interactive mode 
            break;
        case 2: // stdin redirection to script
            if(freopen(argv[1], "r", stdin) == NULL) {
                ERR_CLOSE("Cannot open script %s for reading! %s", argv[1]);
            }
            break;
        default:
            USAGE(argv[0]);
            return EXIT_FAIL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    while((nread = getline(&line, &len, stdin)) > 1) {
        if(line[0] == '#') { continue; }
//        if(line[nread-1] == EOF) { break; }
        if(line[nread-1] == '\n') { line[nread-1] = '\0'; }
        printf("line: %s, bytes read: %ld", line, nread);
        tokenized_line *in_cmd = linetok(line);
        free(line);
        line = NULL;
       
        //printf("%s\n", in_cmd->command);
        pid_t pid = -1;
        int wstatus;
        switch(pid = fork()) {
            case -1:
                ERR_CLOSE("Could not fork during creation of child for %s! %s",
                          in_cmd->command);
            case 0: // TODO MAKE SURE THE CHILD HITS THE SAME CLEANUP DOWN THERE
                if(proc_runner(in_cmd) < 0) {
                    ERR_CLOSE("Error occured while running %s! %s", 
                              in_cmd->command);
                }
                break;
            default:
                if(wait(&wstatus) < -1) {
                    ERR_CLOSE("Error waiting for child, pid %d during %s! %s", 
                              pid, in_cmd->command);
                }
        }
        
        free(in_cmd);
    }
    
    return 0; 
}
