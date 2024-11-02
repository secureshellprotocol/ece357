#ifndef JR_PROC_RUNNER_H
#define JR_PROC_RUNNER_H

#include "tokenizer.h"

#define REDIRECT_FAIL   1
#define CHILD_EXEC_FAIL 127

// Provide with a pre-parsed command from tokenizer.
// Returns REDIRECT_FAIL if any of the redirect operations fail.
// Returns CHILD_EXEC_FAIL if exec fails to run the provided command.
int proc_runner(tokenized_line *in_cmd);

#endif
