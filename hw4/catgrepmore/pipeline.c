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


/*        
            Pipeline diagram
            
                ---------               ---------               ---------
                |       |               |       |               |       |
 filename-->    |  cgm  | --<pipe 1>--> | grep  | --<pipe 2>--> | more  | --> me :D
                |       |               |       |               |       |
                |       |               |       |               |       |
                ---------               ---------               ---------
 */

// Pipeline initializer
// Pipelines must be freed once you finish using it!
// Returns NULL if we cannot allocate a struct
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


// pipeline_bringup() - creates a pipeline, pictured above.
// must provide a file to open, and a pattern to feed grep.
// gets our process in a state ready to run a read_cycle()
bringup_state *pipeline_bringup(char *filename, char *pattern) 
{
    bringup_state *s;

    // Create initial state struct
    if((s = init_state()) == NULL) {
        ERR("init_state: Failed to initialize state! %s");
        goto error;
    }
    s->pattern = pattern;

    if((s->file_in_fd = open(filename, O_RDONLY)) < 0) {
        ERR("pipeline_bringup: Couldn't open %s for reading: %s", filename);
        goto error;
    }
    
    // establish pipes one and two
    int pipe_sz; 
    int pipeline_fds[2];
    if(pipe(pipeline_fds) < 0) {
        ERR("pipeline_bringup: Failed to establish pipes! %s");
        goto error;
    }
    // for testing purposes, we artificially reduce our pipe throughput to 4k
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
    
    // close grep readside, we dont need it
    if(s->grep_in_fd != -1) {
        if(close(s->grep_in_fd) < 0) {
            ERR("Failed to close grep read pipe descriptor! %s");
        } else s->grep_in_fd = -1;
    }

    return s;	// ready for read_cycle()

error: 
    bringdown_state(s);
    return NULL;
}

void read_cycle(bringup_state *s, volatile unsigned int *total_bytes) 
{
    char pipe_write_buffer[4096];
    int r_length, w_length;
    
    // Pipe failures are handled within the loop -- we don't need the signal
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        ERR("read_cycle: Failed to set SIGPIPE to be ignored! %s");
    }
    while((r_length = read(s->file_in_fd, pipe_write_buffer, 4096)) > 0 || (errno == EINTR)) {
        if((w_length = write(s->pipe_out_fd, pipe_write_buffer, r_length)) < r_length || w_length < 0) {
            switch(errno) {
                case EPIPE: // its joever - our pipeline is down!
                    goto exit;
                case EINTR: // small interrupt, get back to work
                    continue;
                default:    // report it and move on -- not a successful write
                    ERR("write: %s");
                    goto exit;
            }
        }
        s->bytes_read += w_length;
        if(total_bytes != NULL) { *total_bytes += w_length; }
    }
exit:
    return;
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
    
    if(signal(SIGPIPE, SIG_DFL) == SIG_ERR) {
        ERR("read_cycle: Failed to reset SIGPIPE to its default handler! %s");
    }
    
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

    // sometimes we jump here if any part of our bringup enters an inconsistent
    // state -- we ensure that all opened fd's get mopped up on a spill
    
    bringdown_read(s);      // implicitly brings down pipe 1
    bringdown_pipe_2(s);


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
        case 0: // grep bringup
            // plumb parent's read_cycle to grep's input (pipe 1)
            if(dup2(s->grep_in_fd, STDIN_FILENO) < 0) {
                ERR("grep_bringup: failed to dup for grep input! %s");
                goto error;
            }
            if(dup2(s->grep_out_fd, STDOUT_FILENO) < 0) {
                ERR("grep_bringup: failed to dup for grep output! %s");
                goto error;
            }
            
            // grep doesn't need to see the rest of state
            bringdown_state(s);
            execlp("grep", "grep", s->pattern, NULL);
            
            // exec failed
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
        case 0: // more bringup
            // plumb grep's output to more's input
            if(dup2(s->more_in_fd, STDIN_FILENO) < 0) {
                ERR("more_bringup: failed to dup for more input! %s");
                goto error;
            }
            
            // more doesn't need to see the rest of state
            bringdown_state(s);
            execlp("more", "more", "-24", NULL);

            // exec failed
            ERR("more_bringup: exec failed! %s");
            goto error;
        default:
            // close pipe 2 --  parent does not need that
            bringdown_pipe_2(s);
            return childpid;
    }
error:
    return EXIT_FAIL; 
}
