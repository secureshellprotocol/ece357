#include <stdlib.h>
#include <unistd.h>

#include "macros.h"
#include "utils.h"

// cd() - Change shell directory
//  returns EXIT_OK on success
//  returns EXIT_CD_FAIL if we couldn't change dir. Refer to `errno`.
int cd(unsigned int argc, char **argv)
{
    char *target_dir;
    switch(argc) {
        case 1:
            target_dir = getenv("HOME");
            if(target_dir == NULL) {
                ERR_CONT("%s: Could not detect $HOME! defaulting to /",
                    argv[0]);
                target_dir = "/";
            }
            break;
        default:
            target_dir = argv[1];
            break;
    }
    if(chdir(target_dir) < 0) {
        ERR_CONT("Failed to change dir to %s! %s", target_dir);
        return EXIT_CD_FAIL;
    }
    return EXIT_OK;
}

// pwd() - return shell's current pwd
//  prints pwd to stdout
void pwd() 
{
    char pwdbuf[256];
    fprintf(stdout, "%s\n", getcwd(pwdbuf, 256));
    return;
}
