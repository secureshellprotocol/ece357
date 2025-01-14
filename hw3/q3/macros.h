#ifndef JR_MACROS_H
#define JR_MACROS_H
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define EXIT_OK             0
#define EXIT_FAIL           -1

// Macro to report an error to 'stderr' AND CLOSE AS A FAILURE.
// Include a trailing '%s' in 'message' to also report errno.
#define ERR_CLOSE(message, ...) \
    fprintf(stderr, message"\n", ## __VA_ARGS__, strerror(errno));\
    return EXIT_FAIL;

// Macro to report an error to 'stderr' and continue.
// Include a trailing '%s' in 'message' to also report errno.
#define ERR_CONT(message, ...) \
    fprintf(stderr, message"\n", ## __VA_ARGS__, strerror(errno));

// Same as ERR_CONT macro, however, theres NO NewLine.
#define ERR_CONT_NONL(message, ...) \
    fprintf(stderr, message, ## __VA_ARGS__, strerror(errno));

#endif
