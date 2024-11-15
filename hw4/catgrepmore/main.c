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

sigjmp_buf readloop_jmp_buf;

// SIGUSR2 handler -- breaks from read_cycle to enter bringdown and prompt our
// next file
void skip_handler(int s)
{
    siglongjmp(readloop_jmp_buf, 1);
}

// SIGUSR1 handler -- reports the global stats from our state cycles
void report_stats_handler(int s)
{
    ERR("\n\n----------Begin Status Report----------\n");
    ERR("Number of files processed: %d\n", files_read);
    ERR("Total number of bytes processed: %d\n", total_reported_bytes);
    ERR("\n-----------End Status Report-----------\n\n");
}

// Signal handler override establisher -- maps `signo` to run `handler`
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
    
    // map SIGUSR1 to report global stats
    if(establish_signal_overrides(SIGUSR1, report_stats_handler) < 0) {
        goto error;
    }
    // map SIGUSR2 to skip current file
    if(establish_signal_overrides(SIGUSR2, skip_handler) < 0) {
        goto error;
    }
   
    for(int i = 2; i < argc; i++) {
        
        // bring up pipeline
        if((s = pipeline_bringup(argv[i], argv[1])) == NULL) {
            ERR("%s: pipeline initialization failed!", argv[0]);
            goto error;
        }
        
        ERR("\n\n>>>\t>>>\t>>>\t>>>Read file %d\n\n", files_read);
        
        // start busyloop
        switch(sigsetjmp(readloop_jmp_buf, 1)) {
            case 0:
                read_cycle(s, &total_reported_bytes);
                break;
            default: // user sent SIGUSR2 -- quit early! jumped here.
                // forcibly stop pipeline -- induce broken pipe by gracefully
                
                // killing `more` and causing a broken pipe
			    // we kill grep first to avoid an annoying broken pipe error
                if(kill(s->grep_pid, SIGINT) < 0) {
                    ERR("kill: %s");
                }
                if(kill(s->more_pid, SIGINT) < 0) {
                    ERR("kill: %s");
                }
                if(files_read + 1 == argc - 1) {
                    ERR("\n\n*** SIGUSR2 received! Quitting");
                } else {
                    ERR("\n\n*** SIGUSR2 received! Moving on to file #%d\n", files_read + 1);
                }
            	break;
        }
        
        // clean up state -- close all lingering fds, and then wait for children
        bringdown_read(s);
        unsigned int grep_wstatus, more_wstatus;
        while(waitpid(s->grep_pid, &grep_wstatus, 0) > 0 || (errno == EINTR));
        while(waitpid(s->more_pid, &more_wstatus, 0) > 0 || (errno == EINTR));
        
        bringdown_state(s);

        files_read++;
    }
    return EXIT_OK;

error: // something unexpected happened in our pipeline
    bringdown_state(s);
    return EXIT_FAIL;
}
