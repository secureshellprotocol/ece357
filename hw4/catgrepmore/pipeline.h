#ifndef JR_PIPELINE_H
#define JR_PIPELINE_H

#include <setjmp.h>

// State struct for our pipeline -- from bringup to bringdown!
// Maintains state on whether or not a particular fd is open, and what our
// children look like. 
// We collect stats on a given file page operation
typedef struct bringup_state_t {
    // file descriptors for each pipe
    int file_in_fd;
    int pipe_out_fd, grep_in_fd;    // pipe 1
    int grep_out_fd, more_in_fd;    // pipe 2

    int grep_pid, more_pid;         // pids of children

    unsigned int bytes_read;        // bytes read from file

    char *pattern;                  // pattern fed to grep
} bringup_state;

bringup_state *pipeline_bringup(char *filename, char *pattern);

bringup_state *init_state();
void bringdown_pipe_1(bringup_state *s);
void bringdown_pipe_2(bringup_state *s);
void bringdown_read(bringup_state *s);
void bringdown_state(bringup_state *s);

int grep_bringup(bringup_state *s);
int more_bringup(bringup_state *s);

void read_cycle(bringup_state *s, volatile unsigned int *total_bytes);

#endif
