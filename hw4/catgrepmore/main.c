#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "macros.h"
#include "pipeline.h"

#define USAGE(name) \
    printf("Usage: %s pattern infile1 [...infile2...]\n", name); return EXIT_FAIL;

int main(int argc, char *argv[]) 
{
    if(argc < 3) { USAGE(argv[0]); } // --> EXIT_FAIL

    unsigned int files_read = 0;
    unsigned int total_reported_bytes = 0;
    bringup_state *s = NULL;
    
    for(int i = 2; i < argc; i++) {
        // bring up pipeline

        if((s = pipeline_bringup(argv[i], argv[1])) == NULL) {
            ERR("%s: pipeline initialization failed!", argv[0]);
            goto error;
        }

        // start busyloop
        if(read_cycle(s) == EXIT_FAIL) {
            ERR("%s: read cycle failure!");
            goto error;
        }
        
        files_read++;
        total_reported_bytes += s->bytes_read;

        ERR("\n\n>>>\t>>>\t>>>\t>>>Read file %d\n\n", files_read);

        // clean up
        bringdown_state(s);
    }
    return EXIT_OK;

error:
    bringdown_state(s);
    return EXIT_FAIL;
}
