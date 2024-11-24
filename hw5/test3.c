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

int main(int argc, char *argv[]) {

    int fd = open(".", O_RDWR | __O_TMPFILE);
    if (fd == -1) {
        fprintf(stderr, "Error encountered while creating the test file\n");
        return 255;
    }
    if (write(fd, "A", 1) == -1) {
        fprintf(stderr, "Error encountered while writing to the test file\n");
        close(fd);
        return 255;
    }
    if (ftruncate(fd, _SC_PAGE_SIZE) == -1) {
        fprintf(stderr, "Error encountered while setting the test file size\n");
        close(fd);
        return 255;
    }

    char *addr = mmap(NULL, _SC_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "Error encountered while creating mmap\n");
        perror(strerror(errno));
        return 255;
    }
    addr[0] = 'B';

    lseek(fd, 0, SEEK_SET);
    char buf[2];
    int n_bytes;
    if ((n_bytes = read(fd, buf, 2)) <= 0) {
        fprintf(stderr, "Error encountered while reading from the test file\n");
        if (n_bytes == -1) {
            perror(strerror(errno));
        }
        close(fd);
        return 255;
    }

    if (buf[0] == 'B') {
        close(fd);
        return 0;
    } else if (buf[0] == 'A') {
        close(fd);
        return 1;
    } else {
        fprintf(stderr, "Unknown error occured");
        close(fd);
        return 255;
    }

}