#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "macros.h"
#include "pipeline.h"

jmp_buf readloop_jmp_buf;

void skip_handler(int s)
{
    longjmp(readloop_jmp_buf, 1);
}

void report_stats_handler(int s)
{
    ERR("\nHEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n\n");
    return;
}

bringup_state *pipeline_bringup(char *filename, char *pattern) 
{
    bringup_state *s;

    // establish initial state
    if((s = init_state()) == NULL) {
        ERR("init_state: Failed to initialize state! %s");
        goto error;
    }
    s->pattern = pattern;

    if((s->file_in_fd = open(filename, O_RDONLY)) < 0) {
        ERR("pipeline_bringup: Couldn't open %s for reading: %s", filename);
        goto error; // this should skip to next file
    }
    
    // establish plumbing
    int pipeline_fds[2];
    if(pipe(pipeline_fds) < 0) {
        ERR("pipeline_bringup: Failed to establish pipes! %s");
        goto error;
    }
    s->pipe_out_fd = pipeline_fds[1];
    s->grep_in_fd = pipeline_fds[0];

    if(pipe(pipeline_fds) < 0) {
        ERR("pipeline_bringup: Failed to establish pipes! %s");
        goto error;
    }
    s->grep_out_fd = pipeline_fds[1];
    s->more_in_fd = pipeline_fds[0];

    // attempt to launch grep
    if((s->grep_pid = grep_bringup(s)) < 0) {
        ERR("pipeline_bringup: Failed to bring up 'grep' child!");
        goto error;
    }
    
    // attempt to launch more
    if((s->more_pid = more_bringup(s)) < 0) {
        ERR("pipeline_bringup: Failed to bring up 'more' child!");
        goto error;
    }
    
    return s;

error: // JK taught me this tactic
    bringdown_state(s);
    return NULL;
}

int read_cycle(bringup_state *s) 
{
    char pipe_write_buffer[4096];
    int readlength;

    // setup SIGUSR2 handler -- skip on demand!
    struct sigaction sa_sigusr2;
    sa_sigusr2.sa_handler = skip_handler;
    sigemptyset(&(sa_sigusr2.sa_mask));
    sigaddset(&(sa_sigusr2.sa_mask), SIGUSR2);
    if(sigaction(SIGUSR2, &sa_sigusr2, NULL) == -1) {
        ERR("sigaction: %s");
        goto error;
    }

    struct sigaction sa_sigusr1;
    sa_sigusr1.sa_handler = report_stats_handler;
    sigemptyset(&(sa_sigusr1.sa_mask));
    sigaddset(&(sa_sigusr1.sa_mask), SIGUSR1);
    if(sigaction(SIGUSR1, &sa_sigusr1, NULL) == -1) {
        ERR("sigaction: %s");
        goto error;
    }

    switch(setjmp(readloop_jmp_buf)) {
        case 0:
            break; // hit the loop
        default:
            ERR("\n*** SIGUSR2 Received! Moving on... \n");
            // bandaid fix -- tell more its time to leave
            if(kill(s->more_pid, SIGINT) < 0) {
                ERR("kill: could not SIGINT more! %s");
                goto error;
            }
            return EXIT_OK; // pretend we hit eof
    }
    
    // this could be more robust .. we arent error checking read!!!!
    while((readlength = read(s->file_in_fd, pipe_write_buffer, 4096)) > 0) {
        if(write(s->pipe_out_fd, pipe_write_buffer, readlength) < 0) {
            ERR("write: %s");
        }
        s->bytes_read += readlength;
    }
    // we end up with a zombie grep.. maybe during bringup, we should establish
    //  async waits for our children?
    return EXIT_OK;
error:
    return EXIT_FAIL;
}

bringup_state *init_state() 
{
    bringup_state *s = malloc(sizeof(bringup_state));
    if(s == NULL) {
        return NULL;
    }
    s->file_in_fd = -1;
    s->pipe_out_fd = -1;
    s->grep_in_fd = -1;
    s->grep_out_fd = -1;
    s->more_in_fd = -1;

    s->grep_pid = -1;
    s->more_pid = -1;

    return s;
}

void bringdown_state(bringup_state *s) 
{
    if(s == NULL) { return; }
    if(s->file_in_fd != -1) {
        if(close(s->file_in_fd) < 0) {
            ERR("Failed to close fd for current open file! %s");
        }
    }
    if(s->pipe_out_fd != -1) {
        if(close(s->pipe_out_fd) < 0) {
            ERR("Failed to close cat write pipe descriptor! %s");
        }
    }
    if(s->grep_in_fd != -1) {
        if(close(s->grep_in_fd) < 0) {
            ERR("Failed to close grep read pipe descriptor! %s");
        }
    }
    if(s->grep_out_fd != -1) {
        if(close(s->grep_out_fd) < 0) {
            ERR("Failed to close grep write pipe descriptor! %s");
        }
    }
    if(s->more_in_fd != -1) {
        if(close(s->more_in_fd) < 0) {
            ERR("Failed to close more read pipe descriptor! %s");
        }
    }

    // wait on the kids
    while(waitpid(s->more_pid, NULL, 0) > 0 || errno==EINTR) {
        if(errno != EINTR && errno != 0) {
            ERR("waitpid: failed to wait for more! %s");
        }
    }
    while(waitpid(s->grep_pid, NULL, 0) > 0 || errno==EINTR) {
        if(errno != EINTR && errno != 0) {
            ERR("waitpid: failed to wait for grep! %s");
        }
    }

    free(s);
    s = NULL;
    return;
}

int grep_bringup(bringup_state *s) 
{
    int childpid;
    switch(childpid = fork()) {
        case -1:
            ERR("grep_bringup: Failed to fork! %s");
            goto error;
        case 0:
            if(dup2(s->grep_in_fd, STDIN_FILENO) < 0) {
                ERR("grep_bringup: failed to dup for grep input! %s");
                goto error;
            }
            if(dup2(s->grep_out_fd, STDOUT_FILENO) < 0) {
                ERR("grep_bringup: failed to dup for grep output! %s");
                goto error;
            }
            bringdown_state(s);
            execlp("grep", "grep", s->pattern, NULL);
            ERR("grep_bringup: exec failed! %s");
            goto error;
        default: 
            return childpid;
    }
error:
    return EXIT_FAIL;
}

int more_bringup(bringup_state *s) 
{
    int childpid;
    switch(childpid = fork()) {
        case -1:
            ERR("more_bringup: Failed to fork! %s");
            goto error;
        case 0:
            dup2(s->more_in_fd, STDIN_FILENO);
            bringdown_state(s);
            execlp("more", "more", NULL);
            ERR("more_bringup: exec failed! %s");
            goto error;
        default:
            return childpid;
    }
error:
    return EXIT_FAIL;   
}
