#include <stdio.h>

#include "macros.h"
#include "pipeline.h"

#define USAGE(name) \
    printf("Usage: %s pattern infile1 [...infile2...]", name); return EXIT_FAIL;

int main(int argc, char *argv[]) 
{
    if(argc < 3) { USAGE(argv[0]); } // --> EXIT_FAIL

    pipeline_bringup(--argc, &(argv[1]));

    return EXIT_OK;
}
