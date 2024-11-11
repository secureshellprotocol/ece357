#define _GNU_SOURCE
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "macros.h"
#include "pipeline.h"


/*  ---------               ---------               ---------
 *  |       |               |       |               |       |
 *  |  cgm  | --<pipe 1>--> | grep  | --<pipe 2>--> | more  |
 *  |       |               |       |               |       |
 *  |       |               |       |               |       |
 *  ---------               ---------               ---------
 */


bringup_state *pipeline_bringup(char *filename, char *pattern) 
{
    bringup_state *s;

    // establish initial state
    // TODO: Should i suppress stderr for grep and more?
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
    
    int pipe_sz; 
    pipe_sz = fcntl(pipeline_fds[1], F_SETPIPE_SZ, 4096);
    s->pipe_out_fd = pipeline_fds[1];
    s->grep_in_fd = pipeline_fds[0];

    if(pipe(pipeline_fds) < 0) {
        ERR("pipeline_bringup: Failed to establish pipes! %s");
        goto error;
    }
    pipe_sz = fcntl(pipeline_fds[1], F_SETPIPE_SZ, 4096);
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
    
    // close redundant file descriptors
    if(s->grep_in_fd != -1) {
        if(close(s->grep_in_fd) < 0) {
            ERR("Failed to close grep read pipe descriptor! %s");
        } else s->grep_in_fd = -1;
    }

    // ready to read

    return s;

error: 
    bringdown_state(s);
    return NULL;
}

void read_cycle(bringup_state *s) 
{
    char pipe_write_buffer[4096];
    int r_length, w_length;
    signal(SIGPIPE, SIG_IGN);
    while((r_length = read(s->file_in_fd, pipe_write_buffer, 4096)) > 0 || errno == EINTR) {
        if((w_length = write(s->pipe_out_fd, pipe_write_buffer, r_length)) < r_length || w_length < 0) {
            switch(errno) {
                case EPIPE: // dont care
                    break;
                default:    // report it and move on
                    ERR("write: %s");
                    break;
            }
        }
        s->bytes_read += r_length;
    }
    return;
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

void bringdown_pipe_1(bringup_state *s) 
{
    if(s == NULL) { return; }

    if(s->pipe_out_fd != -1) {
        if(close(s->pipe_out_fd) < 0) {
            ERR("Failed to close cat write pipe descriptor! %s");
        } else s->pipe_out_fd = -1;
    }
    if(s->grep_in_fd != -1) {
        if(close(s->grep_in_fd) < 0) {
            ERR("Failed to close grep read pipe descriptor! %s");
        } else s->grep_in_fd = -1;
    }
    
    return;
}

void bringdown_pipe_2(bringup_state *s) 
{
    if(s == NULL) { return; }

    if(s->grep_out_fd != -1) {
        if(close(s->grep_out_fd) < 0) {
            ERR("Failed to close grep write pipe descriptor! %s");
        } else s->grep_out_fd = -1;
    }
    if(s->more_in_fd != -1) {
        if(close(s->more_in_fd) < 0) {
            ERR("Failed to close more read pipe descriptor! %s");
        } else s->more_in_fd = -1;
    }
    
    return;
}

void bringdown_read(bringup_state *s) 
{
    if (s == NULL) { return; }

    if(s->file_in_fd != -1) {
        if(close(s->file_in_fd) < 0) {
            ERR("Failed to close input file descriptor! %s");
        } else s->file_in_fd = -1;
    }

    bringdown_pipe_1(s);
    return;
}

void bringdown_state(bringup_state *s) 
{
    if(s == NULL) { return; }

    // ensure we handle fd's if we enter in an inconsistent state
    bringdown_pipe_1(s);
    bringdown_pipe_2(s);
    bringdown_read(s);

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
            bringdown_pipe_2(s);
            return childpid;
    }
error:
    return EXIT_FAIL; 
}
