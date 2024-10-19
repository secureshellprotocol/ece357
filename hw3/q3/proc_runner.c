#include <unistd.h>

#include "macros.h"
#include "tokenizer.h"

//// Checks if its a valid redirect. IE:
////  the char after our redirect op is not \n or \0
////  < in char pos '0'
////  > in char pos '0' or '1'
//void redirect_setup(char *redirv_element){
//    FILE *redir_out = stdout;
//    if(redirv_element[0] == '2') {
//        redir_out = stderr;
//        redirv_element++;
//    }
//    for()
//}

int proc_runner(tokenized_line *in_cmd) 
{
//    // set up redirects
//    for(int i = 0; i < in_cmd->redirc; i++) {
//        if((in_cmd->redirv)[i] == '<') {
//            // redirect stdin to file
//        }
//        if((in_cmd->redirv)[i] == '>') {
//            // redirect stdout to file
//        }
//        if((in_cmd->))
//    }

    execvp((in_cmd->argv)[0], in_cmd->argv);

    ERR_CLOSE("Failed to exec %s! %s", (in_cmd->argv)[0]);
}
