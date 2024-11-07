#include <stdio.h>

#include "macros.h"

#define USAGE(name) \
    printf("Usage: %s pattern infile1 [...infile2...]", name);

int main(int argc, char *argv[]) 
{
    if(argc < 3) {
        USAGE(argv[0]);
        return EXIT_FAIL;
    }

    for(unsigned int i = 2; i < argc; i++) {
        FILE* in;
        if((in = fopen(argv[i], 'r')) == NULL) {
            ERR_CLOSE("%s: Couldn't open %s for reading: %s", argv[0], argv[i]);
        }
        
        // establish plumbing
        int file_to_grep[2];
        if(pipe(file_to_grep) < 0) {
            ERR_CLOSE("%s: Failed to establish pipes! %s", argv[0]);
        }

        int grep_to_more[2];
        if(pipe(grep_to_more) < 0) {
            ERR_CLOSE("%s: Failed to establish pipes! %s", argv[0]);
        } 
        
        // establish 
        int cpid;
        switch(cpid = fork()) {
            case -1:    // fork failed
                ERR_CLOSE("%s: Failed to fork child: %s", argv[0]);
            case 0:     // child
                                
            default:    // parent
        }
    }
}
