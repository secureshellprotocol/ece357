#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define EXIT_OK     0
#define EXIT_FAIL   -1

// Macro to report an error to 'stderr' AND CLOSE AS A FAILURE.
// Include a trailing '%s' in 'message' to also report errno.
#define ERR_CLOSE(message, ...) \
    fprintf(stderr, message"\n", ## __VA_ARGS__, strerror(errno));\
    return EXIT_FAIL;

// Macro to report an error to 'stderr' and continue.
// Include a trailing '%s' in 'message' to also report errno.
#define ERR_CONT(message, ...) \
    fprintf(stderr, message"\n", ## __VA_ARGS__, strerror(errno));

int main(int argc, char *argv[]) {
    
    if (argc > 2) {
        ERR_CLOSE("Too many arguments!");
    }

    // if the provided arg is not a directory, close
    struct stat st;
    int fd;

    stat(argv[1], &st);
    if (!S_ISDIR(st.st_mode)) {
        ERR_CLOSE("%s is not a directory!", argv[1]);
    }

    return EXIT_OK;
}
