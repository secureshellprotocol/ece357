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

    int FILE_SIZE = 1; // >= 1

    // temp file contents:
    // "A" + '\0' * FILE_SIZE-1 + 'X' + '\0'

    int fd = open(".", O_RDWR | __O_TMPFILE, 0666);
    if (fd == -1) {
        fprintf(stderr, "Error encountered while creating the test file\n");
        perror(strerror(errno));
        close(fd);
        return 255;
    }
    if (write(fd, "A", FILE_SIZE) == -1) {
        fprintf(stderr, "Error encountered while writing to the test file\n");
        perror(strerror(errno));
        close(fd);
        return 255;
    }

    char *addr = mmap(NULL, ((FILE_SIZE/4096)+1)*4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "Error encountered while creating mmap\n");
        perror(strerror(errno));
        return 255;
    }

    addr[FILE_SIZE] = 'X';

    if (lseek(fd, FILE_SIZE+2, SEEK_SET) == -1) {
        perror("Error encountered while extending file");
        perror(strerror(errno));
        close(fd);
        return 255;
    }
    if (write(fd, "\0", 1) == -1) {
        perror("Error encountered while finalizing file extension");
        perror(strerror(errno));
        close(fd);
        return 255;
    }

    char buffer[FILE_SIZE+2];
    lseek(fd, 0, SEEK_SET);
    if (read(fd, buffer, FILE_SIZE+2) == -1) {
        perror("Failed to read file");
        perror(strerror(errno));
        close(fd);
        return 255;
    }

    if (buffer[FILE_SIZE] == 'X') {
        close(fd);
        return 0;
    }

    return 1;

}