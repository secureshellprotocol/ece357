#define _GNU_SOURCE //https://stackoverflow.com/questions/25411892/f-setpipe-sz-undeclared
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "macros.h"
#include "pipeline.h"

#define USAGE(name) \
    printf("Usage: %s pattern infile1 [...infile2...]\n", name); return EXIT_FAIL;

unsigned int volatile files_read = 1;
unsigned int volatile total_reported_bytes = 0;

jmp_buf readloop_jmp_buf;

void skip_handler(int s)
{
    longjmp(readloop_jmp_buf, 1);
}

void report_stats_handler(int s)
{
    ERR("\n\n----------Begin Status Report----------\n");
    ERR("Number of files processed: %d\n", files_read);
    ERR("Total number of bytes processed: %d\n", total_reported_bytes);
    ERR("\n-----------End Status Report-----------\n\n");
}

int establish_signal_overrides(int signo, void (*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags=0;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), signo);
    if(sigaction(signo, &sa, NULL) == -1) {
        ERR("establish_signal_overrides: sigaction: %s");
        return EXIT_FAIL;
    }
    return EXIT_OK;
}

int main(int argc, char *argv[]) 
{
    if(argc < 3) { USAGE(argv[0]); } // --> EXIT_FAIL

    bringup_state *s = NULL;
    
    for(int i = 2; i < argc; i++) {
        
        // bring up pipeline
        if((s = pipeline_bringup(argv[i], argv[1])) == NULL) {
            ERR("%s: pipeline initialization failed!", argv[0]);
            goto error;
        }
        
        int grep_pid = s->grep_pid;
        int more_pid = s->more_pid;
        
        if(establish_signal_overrides(SIGUSR1, report_stats_handler) < 0) {
            goto error;
        }
        if(establish_signal_overrides(SIGUSR2, skip_handler) < 0) {
            goto error;
        }

        ERR("\n\n>>>\t>>>\t>>>\t>>>Read file %d\n\n", files_read);
        
        // start busyloop
        switch(setjmp(readloop_jmp_buf)) {
            case 0:
                read_cycle(s);
                break;
            default:
                ERR("\n\n*** SIGUSR2 received! Moving on to file #%d\n", files_read);
                // forcibly stop pipeline, inducing a broken pipe
                kill(grep_pid, SIGINT);
                kill(more_pid, SIGINT);
                goto file_skip;
        }
        
        total_reported_bytes += s->bytes_read;
        
file_skip:

        bringdown_read(s);
        unsigned int grep_wstatus, more_wstatus;
        while(waitpid(more_pid, &more_wstatus, 0) > 0 || (errno == EINTR));
        while(waitpid(grep_pid, &grep_wstatus, 0) > 0 || (errno == EINTR));
        // clean up
        bringdown_state(s);

        files_read++;
    }
    return EXIT_OK;

error:
    bringdown_state(s);
    return EXIT_FAIL;
}
