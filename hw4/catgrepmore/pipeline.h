#ifndef JR_PIPELINE_H
#define JR_PIPELINE_H

#include <setjmp.h>

typedef struct bringup_state_t {
    int file_in_fd, pipe_out_fd;
    int grep_in_fd, grep_out_fd;
    int more_in_fd;

    int grep_pid;
    int more_pid;

    unsigned int bytes_read;

    char *pattern;
} bringup_state;

extern jmp_buf readloop_jmp_buf;
void skip_handler(int signum);
void report_stats_handler(int signum);

bringup_state *pipeline_bringup(char *filename, char *pattern);

bringup_state *init_state();
void bringdown_state(bringup_state *s);

int grep_bringup(bringup_state *s);
int more_bringup(bringup_state *s);

int read_cycle(bringup_state *s);

#endif
