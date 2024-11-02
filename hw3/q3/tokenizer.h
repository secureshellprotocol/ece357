#ifndef JR_TOKENIZER_H
#define JR_TOKENIZER_H

#define VECTOR_SIZE 50      // Defines max size of argv and redirv.

// Tokenizer data struct -- keeps some stats on the incoming command
typedef struct tokenizer_data_t {
    unsigned int argc;          // # of elems in argv
    char *argv[VECTOR_SIZE];    // arg vector
    unsigned int redirc;        // # of elems in redirv
    char *redirv[VECTOR_SIZE];  // redirect vector
} tokenized_line;

// Classifies a given token as either a:
//  1 - redirect
//  0 - argument
int classify(char *token);

// Tokenizes a line, and returns information on our target's argc and argv
// Returned struct must be free()'d.
//  Returns NULL on failure.
tokenized_line *linetok(char *instring);

#endif
