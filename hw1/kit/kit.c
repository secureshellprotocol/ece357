#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define EXIT_OK 0
#define EXIT_FAIL -1

// Usage statement, printed upon prescense of bad args
#define USAGE() \
    fprintf(stdout, "Usage: kit [-o outfile] [-b buffer-size] infile1 [...infile2....] \
                   \n       kit [-o outfile] [-b buffer-size]\n");

// https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
// Helper definition to report an error and close. Just handles output to stderr,
// and return a FAIL in the context.
// ONLY reports the error if a trailing `%s` is provided, with no matching var. 
// Otherwise, just prints the message and closes. 
// ALWAYS prints a newline.
//	Hak -- I reverted the change since it broke my error reporting for my
//	`-b` flag. I think handling the choice and leaving it to the writer
//	is elegant enough. As far as im aware: not including `%s` for strerror 
//	is not particularly harmful.
#define ERR_CLOSE(message, ...) \
    fprintf(stderr, message "\n", __VA_ARGS__, strerror(errno));\
    return EXIT_FAIL; 

int main(int argc, char *argv[])
{
    int opt;
    unsigned int buf_size = BUF_SIZE; // size of read-in buffer
    
    int outfile_fd = STDOUT_FILENO;
    char *outfile_name = "stdout";

    while((opt = getopt(argc, argv, "o:b:")) != -1) {
        switch(opt) {
        case 'o': // open and set outfile away from stdout
            outfile_fd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if(outfile_fd < 0) {
                ERR_CLOSE("Failed to open %s for writing: %s", optarg);
            }
            outfile_name = optarg;
            break;
        case 'b': // parse for and set a size for our buffer
            buf_size = atoi(optarg);
            if(buf_size <= 0) {
                ERR_CLOSE("Invalid buffer size provided: %s", optarg);
            }
            break;
        default:
            USAGE();
            return EXIT_FAIL;
        } 
    }

    char *io_buffer = (char *) malloc(sizeof(char) * buf_size);
    do {
        int bytes_read, bytes_wrote, infile_fd = STDIN_FILENO;
        
        // check if we are less than optind first -- this covers the case where
        // we pass in no `infile`s, and instead just would like to read stdin
        if(optind < argc && strcmp(argv[optind], "-") != 0) {
            if((infile_fd = open(argv[optind], O_RDONLY)) < 0) {
                ERR_CLOSE("Failed to open %s for reading: %s", argv[optind]);
            }
        }
        
        while((bytes_read = read(infile_fd, io_buffer, buf_size)) > 0) {
            bytes_wrote = write(outfile_fd, io_buffer, bytes_read);
            if(bytes_wrote != bytes_read) {
                ERR_CLOSE("Failed to write %s to %s: %s", argv[optind], 
                    outfile_name);
            }
        }
        
        if(infile_fd != STDIN_FILENO && close(infile_fd) < 0) {
            ERR_CLOSE("Failed to close %s: %s", argv[optind]);
        }

        optind++;
    } while(optind < argc); 

    if(outfile_fd != STDOUT_FILENO && close(outfile_fd) < 0) {
        ERR_CLOSE("Failed to close %s: %s", outfile_name);
    }
 
    free(io_buffer);
    return EXIT_OK;
}
