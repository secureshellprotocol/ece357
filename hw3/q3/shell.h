#ifndef JR_SHELL_H
#define JR_SHELL_H

#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>

#define USAGE(name) fprintf(stdout, "Usage: %s {script}", name)

// converts timeval to seconds, as a double.
double timeval2sec(struct timeval input);

// reports status of our child to stdout.
// returns the exit code if the child exited successfully, or returns the
//  signal number if the child was killed by a signal
int report_child_status(pid_t pid, unsigned int wstatus, 
                        struct rusage *ru, double real);

int main(int argc, char *argv[]);

#endif 
