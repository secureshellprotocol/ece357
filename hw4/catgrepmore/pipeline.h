#ifndef JR_PIPELINE_H
#define JR_PIPELINE_H

typedef struct bringup_state_t {
    int file_in_fd, pipe_out_fd;
    int grep_in_fd, grep_out_fd;
    int more_in_fd;

    char *pattern;
} bringup_state;

int pipeline_bringup(int argc, char *argv[]);

bringup_state *init_state();
void bringdown_state(bringup_state *s);

int grep_bringup(bringup_state *s);
int more_bringup(bringup_state *s);

void read_cycle(bringup_state *s);

#endif
