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
    
    return s;

error: // JK taught me this tactic
    bringdown_state(s);
    return NULL;
}

int read_cycle(bringup_state *s) 
{
    char pipe_write_buffer[4096];
    int readlength;

    // this could be more robust .. we arent error checking read!!!!
    while((readlength = read(s->file_in_fd, pipe_write_buffer, 4096)) > 0 || (errno == EINTR)) {
        if(write(s->pipe_out_fd, pipe_write_buffer, readlength) < 0 && errno != EINTR) {
            ERR("write: %s");
        }
        s->bytes_read += readlength;
    }
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

void bringdown_pipes(bringup_state *s) 
{
    if(s == NULL) { return; }

    if(s->file_in_fd != -1) {
        if(close(s->file_in_fd) < 0) {
            ERR("Failed to close fd for current open file! %s");
        } else s->file_in_fd = -1;
    }
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


void bringdown_state(bringup_state *s) 
{
    if(s == NULL) { return; }

    bringdown_pipes(s);

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
