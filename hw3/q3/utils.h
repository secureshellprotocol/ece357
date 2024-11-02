#ifndef JR_UTILS_H
#define JR_UTILS_H

#define EXIT_CD_FAIL 1

// cd() - Change shell directory
//  returns EXIT_OK on success
//  returns EXIT_CD_FAIL if we couldn't change dir. Refer to `errno`. 
int cd(unsigned int argc, char **argv);

// pwd() - return shell's current pwd
//  prints pwd to stdout
void pwd();
#endif
