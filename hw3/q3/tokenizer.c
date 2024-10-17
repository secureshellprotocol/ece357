#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"
#include "tokenizer.h"

tokenized_line *linetok(char *instring) 
{
    strtok(instring, "\n"); 

    tokenized_line *target = malloc(sizeof(tokenized_line));
    
    if(target == NULL) {
        ERR_CONT("Failed to parse tokens of %s! %s", &instring);
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
        ERR_CONT("Failed to parse provided command! %s");
        return NULL;
    }
    target->command = strdup(token); // TODO: clean this up, it uses malloc

    while((token = strtok_r(NULL, " ", &saveptr)) != NULL) {
        unsigned int i = 0;
        char c;
        while((c = token[i]) != '\0') {
            if(c == '<' || c == '>') {
                target->redirv[target->redirc] = strdup(token);
                target->redirc++;
                break;
            }
            i++;
        }
        if(c == '\0') {
            target->argv[target->argc] = strdup(token);
            target->argc++;
        }
    }

    return target;
}
