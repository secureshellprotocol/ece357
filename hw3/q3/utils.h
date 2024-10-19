#ifndef JR_UTILS_H
#define JR_UTILS_H
// cd(char *) - Change shell directory
//  returns EXIT_OK on success
//  returns EXIT_FAIL if we couldn't change dir. Refer to `errno`. directory
//    state IS preserved
int cd(char *target_dir);

// pwd() - return shell's current pwd
//  prints pwd to stdout
void pwd();

// exit(int) - exit shell
//  Sends a SIGINT to the parent, waits for them to die, and then returns with a
//  provided exit code.
void exit(int code);
#endif
