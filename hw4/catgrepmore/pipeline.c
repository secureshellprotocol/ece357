#include <stdio.h>

#include "macros.h"

int pipeline_bringup(int argc, char *argv[]) 
{
    for(unsigned int i = 1; i < argc; i++) {
        FILE* in;
        if((in = fopen(argv[i], 'r')) == NULL) {
            ERR_CLOSE("pipeline_bringup: Couldn't open %s for reading: %s", 
                    argv[i]);
        }
        
        // establish plumbing
        int file_to_grep[2];
        if(pipe(file_to_grep) < 0) {
            ERR("%s: Failed to establish pipes! %s", argv[0]);
            goto pipefail1;
        }

        int grep_to_more[2];
        if(pipe(grep_to_more) < 0) {
            ERR("%s: Failed to establish pipes! %s", argv[0]);
            goto pipefail2;
        } 

        grep_bringup();
        more_bringup();
    }

    return EXIT_OK;

grepfail:
pipefail2:
pipefail1:
error: // JK taught me this tactic
    return EXIT_FAIL;

}

int 
