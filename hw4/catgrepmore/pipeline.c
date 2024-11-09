#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "macros.h"
#include "pipeline.h"

int pipeline_bringup(int argc, char *argv[]) 
{
    bringup_state *s;
    for(unsigned int i = 2; i < argc; i++) {
        // establish initial state
        if((s = init_state()) == NULL) {
            ERR("init_state: Failed to initialize state! %s");
            goto error;
        }
        s->pattern = argv[1];

        if((s->file_in_fd = open(argv[i], O_RDONLY)) < 0) {
            ERR("pipeline_bringup: Couldn't open %s for reading: %s", argv[i]);
            goto error; // this should skip to next file
        }
        
        // establish plumbing
        int pipeline_fds[2];
        if(pipe(pipeline_fds) < 0) {
            ERR("pipeline_bringup: Failed to establish pipes! %s\n");
            goto error;
        }
        s->pipe_out_fd = pipeline_fds[1];
        s->grep_in_fd = pipeline_fds[0];

        if(pipe(pipeline_fds) < 0) {
            ERR("pipeline_bringup: Failed to establish pipes! %s\n");
            goto error;
        }
        s->grep_out_fd = pipeline_fds[1];
        s->more_in_fd = pipeline_fds[0];

        int grep_pid = -1;
        if((grep_pid = grep_bringup(s)) < 0) {
            ERR("pipeline_bringup: Failed to bring up 'grep' child! %s\n");
            goto error;
        }
        
        int more_pid = -1;
        if((more_pid = more_bringup(s)) < 0) {
            ERR("pipeline_bringup: Failed to bring up 'more' child!\n");
            goto error;
        }

        read_cycle(s);

        // clean up our file descriptors
        bringdown_state(s);
        
        unsigned int grep_wstatus;
        waitpid(grep_pid, &grep_wstatus, 0);
        unsigned int more_wstatus;
        waitpid(more_pid, &more_wstatus, 0);
    }

    return EXIT_OK;

error: // JK taught me this tactic
    bringdown_state(s);
    return EXIT_FAIL;
}

void read_cycle(bringup_state *s) {
    char pipe_write_buffer[4096];
    int readlength;

    while((readlength = read(s->file_in_fd, pipe_write_buffer, 4096)) > 2) {
        if(write(s->pipe_out_fd, pipe_write_buffer, readlength) < 0) {
            ERR("write failed %s");
        }
    }
    return;
}

bringup_state *init_state() {
    bringup_state *s = malloc(sizeof(bringup_state));
    if(s == NULL) {
        return NULL;
    }
    s->file_in_fd = -1;
    s->pipe_out_fd = -1;
    s->grep_in_fd = -1;
    s->grep_out_fd = -1;
    s->more_in_fd = -1;

    return s;
}

void bringdown_state(bringup_state *s) {
    if(s ==  NULL) { return; }
    if(s->file_in_fd != -1) {
        close(s->file_in_fd);
    }
    if(s->pipe_out_fd != -1) {
        close(s->pipe_out_fd);
    }
    if(s->grep_in_fd != -1) {
        close(s->grep_in_fd);
    }
    if(s->grep_out_fd != -1) {
        close(s->grep_out_fd);
    }
    if(s->more_in_fd != -1) {
        close(s->more_in_fd);
    }

    free(s);
    s = NULL;
    return;
}

int grep_bringup(bringup_state *s) {
    int childpid;
    switch(childpid = fork()) {
        case -1:
            ERR("grep_bringup: Failed to fork! %s\n");
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
            // erm.. shouldnt have gotten here
            ERR("grep_bringup: exec failed! %s");
            goto error;
        default: 
            return childpid;
    }
error:
    return EXIT_FAIL;
}

int more_bringup(bringup_state *s) {
    int childpid;
    switch(childpid = fork()) {
        case -1:
            ERR("more_bringup: Failed to fork! %s\n");
            goto error;
        case 0:
            dup2(s->more_in_fd, STDIN_FILENO);
            bringdown_state(s);
            execlp("more", "more", NULL);
            // erm.. shouldnt have gotten here
            ERR("more_bringup: exec failed! %s");
            goto error;
        default:
            return childpid;
    }
error:
    return EXIT_FAIL;   
}
