#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

/* Assume the program below was invoked with the following command line:
./program <in.txt >out.txt 2>&1
Prior to invocation, the files in.txt and out.txt exist and each
is 80 bytes long. We have write permission on the current directory.
Assume that all systems calls in the code below succeed
*/
int main()
{
    int pid,fd;
    write(1,"1234",4);
    switch(pid=fork())
    {
        case -1: perror("fork");return -1;
        case 0: dup2(2,3);break;
        default: fd=open("out.txt",O_WRONLY|O_APPEND);dup2(fd,1);
            printf("ABC");
            fflush(stdout);
    }
/* Sketch the file descriptor tables of both parent and child processes
when they have both reached this point. Show the connection between
each open file descriptor in each process and the various struct file
instances with an arrow pointing from the former to the latter.
Show the values of the f_mode, f_flags, f_count and f_pos fields.
Represent f_mode and f_flags symbolically, e.g. O_RDONLY
It is sufficient to denote the inodes by the name of the file that
they represent, similar to how these diagrams were presented in the
lecture notes and in class.
*/
    for(;;)
    /* endless loop */;
}
