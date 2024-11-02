#include <fcntl.h>
#include <unistd.h>

#include "macros.h"
#include "proc_runner.h"
#include "tokenizer.h"

// Provide with a pre-parsed command from tokenizer.
// Returns REDIRECT_FAIL if any of the redirect operations fail
// Returns CHILD_EXEC_FAIL if exec fails to run the provided command.
int proc_runner(tokenized_line *in_cmd) 
{
    int out_fd = STDOUT_FILENO;
    mode_t inopts = O_RDONLY;
    mode_t outopts = O_WRONLY | O_CREAT | O_TRUNC;

    for(int i = 0; i < in_cmd->redirc; i++) {
        if((in_cmd->redirv)[i][0] == '<') {
            int fd = open(&((in_cmd->redirv)[i][1]), inopts);
            if(fd < 0) {
                ERR_CONT("Failed to open %s! %s", 
                        &((in_cmd->redirv)[i][1]));
                return REDIRECT_FAIL;
            }
            if(dup2(fd, STDIN_FILENO) < 0){
                ERR_CONT("Failed to dup %s! %s", 
                        &((in_cmd->redirv)[i][1]));
                return REDIRECT_FAIL;
            }
            continue;
        }

        // keep track of an offset during stdout parse
        //  since we know where '>' can be, we only want to pass actual filename
        //  characters to our `open()` call
        int offset = 0;
        if((in_cmd->redirv)[i][0] == '2') {
            out_fd = STDERR_FILENO;
            offset++;
        }
        if((in_cmd->redirv)[i][0+offset] == '>') {
            if((in_cmd->redirv)[i][1+offset] == '>'){
                offset++;
                outopts = O_WRONLY | O_CREAT | O_APPEND;
            }
            int fd = open(&((in_cmd->redirv)[i][1+offset]), outopts, 0666);
            if(fd < 0) {
                ERR_CONT("Failed to open %s! %s", 
                        &((in_cmd->redirv)[i][1]));
                return REDIRECT_FAIL;
            }           
            if(dup2(fd, out_fd) < 0) {
                ERR_CONT("Failed to dup %s! %s",
                        &((in_cmd->redirv)[i][1]));
                return REDIRECT_FAIL;
            }
            if(close(fd) < 0) {
                ERR_CONT("Failed to close old fd for %s! %s", 
                        &((in_cmd->redirv)[i][1]));
                return REDIRECT_FAIL;
            }
        }
    }

    execvp((in_cmd->argv)[0], in_cmd->argv);

    ERR_CONT("Failed to exec %s! %s", (in_cmd->argv)[0]);
    return CHILD_EXEC_FAIL;
}

