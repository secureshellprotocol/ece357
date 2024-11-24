#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>

void signal_handler(int sig) {
    exit(sig);
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    for (int i = 0; i < 32; i++) {
        sigaction(i, &sa, NULL);
    }

    char *addr = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        return 254;
    }

    addr[0] = 'A';

    if (addr[0] == 'A') {
        return 0;
    }

    return 255;
}