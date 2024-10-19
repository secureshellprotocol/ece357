#ifndef JR_TOKENIZER_H
#define JR_TOKENIZER_H

#define VECTOR_SIZE 5       // Defines max size of argv and redirv before
                            // realloc. Each realloc will be (n+1) * VECTOR_SIZE
                            // in size, where n is 
                            //  (# elems in vector) % VECTOR_SIZE

typedef struct tokenizer_data_t {
    unsigned int argc;      // # of elems in argv
    char **argv;            // arg vector
    unsigned int redirc;    // # of elems in redirv
    char **redirv;          // redirect vector
} tokenized_line;

tokenized_line *linetok(char *instring);

#endif
