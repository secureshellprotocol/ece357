#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"
#include "tokenizer.h"

// Classifies a given token as either a:
//  1 - redirect
//  0 - argument
int classify(char *token) {
    unsigned int i = 0;
    char c;
    for(int i = 0; (c = token[i]) != '\0'; i++) {
        if(c == '<' || c == '>') {
            if(token[i+1] == '\0') { return -1; } //no filename provided
            return 1;
        }
    }
    return 0;
}

// Tokenizes a line, and returns information on our target's argc and argv
// Returned struct must be free()'d.
//  Returns NULL on failure.
tokenized_line *linetok(char *instring) 
{
    tokenized_line *target = calloc(1, sizeof(tokenized_line));

    if(target == NULL) {
        ERR_CONT("Failed to parse tokens of \"%s\"! %s", &instring);
        return NULL;
    }
    
    char *token, *saveptr = NULL;
    token = strtok_r(instring, " ", &saveptr);
    if(token == NULL) {
        ERR_CONT("Failed to parse \"%s\"! %s", instring);
        return NULL;
    }
    (target->argv)[0] = strdup(token);
    target->argc++;

    while((token = strtok_r(NULL, " ", &saveptr)) != NULL) {
        switch(classify(token)) {
            case 1: // redirect
                (target->redirv)[target->redirc] = strdup(token);
                (target->redirc)++;
                if(target->redirc == VECTOR_SIZE) {
                    errno = E2BIG;
                    return NULL;
                }
                break;
            case 0: // argument
                (target->argv)[target->argc] = strdup(token);
                (target->argc)++;
                if(target->argc == VECTOR_SIZE) {
                    errno = E2BIG;
                    return NULL;
                }
                break;
            default: // bad arg
                break;
        }
    }
    
    return target;
}
