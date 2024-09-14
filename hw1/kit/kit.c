#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void usage()
{
    fprintf(stderr, "Usage: kit [-o outfile] infile1 [...infile2....]\
                   \n       kit [-o outfile]");
    exit(-1);
}

int main(int argc, char *argv[])
{
    int n, opt;
    while((opt = getopt(argc, argv, "o:b:")) != -1) {
        switch(opt) {
        case 'o':
            n = atoi(optarg);
            break;
        case 'b':
            printf("not implemented!");
            exit(1);
        default:
            usage(); // exits
        }
    }
    
    printf("n=%d", n);

    return 0;
}
