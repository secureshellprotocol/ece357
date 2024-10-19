#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"
#include "tokenizer.h"

tokenized_line *linetok(char *instring) 
{
    tokenized_line *target = malloc(sizeof(tokenized_line));
 
    if(target == NULL) {
        ERR_CONT("Failed to parse tokens of \"%s\"! %s", &instring);
        return NULL;
    }

    // TODO: make these reallocable
    target->argc = 0;
    target->argv = malloc(sizeof(char *) * VECTOR_SIZE);
    target->redirc = 0;
    target->redirv = malloc(sizeof(char *) * VECTOR_SIZE);
    
    char *token, *saveptr = NULL;
    token = strtok_r(instring, " ", &saveptr);
    if(token == NULL) {
        ERR_CONT("Failed to parse \"%s\"! %s", instring);
        return NULL;
    }
    (target->argv)[0] = strdup(token);
    target->argc++;

TERMPARSE:
    while((token = strtok_r(NULL, " ", &saveptr)) != NULL) {
        unsigned int i = 0;
        char c;
        for(int i = 0; (c = token[i+1]) != '\0'; i++) {
            if(c == '<' || c == '>') {
                (target->redirv)[target->redirc] = strdup(token);
                (target->redirc)++;
                goto TERMPARSE; // TODO BAD SOLN
            }
        }
        (target->argv)[target->argc] = strdup(token);
        (target->argc)++;
    }
    
    return target;
}
