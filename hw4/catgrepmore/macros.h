#ifndef JR_MACROS_H
#define JR_MACROS_H
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define EXIT_OK             0
#define EXIT_FAIL           -1

// Macro to report an error to 'stderr' and continue.
// We are NOT responsible for cleanup.
#define ERR(message, ...) \
    fprintf(stderr, message, ## __VA_ARGS__, strerror(errno));

#endif
