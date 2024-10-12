#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int f1()
{
    static int i;
    printf("%d\n",i);
    i++;
}

int main()
{
    int ws;
    f1();
    
    if (fork()==0)
        f1();
    
    f1();
    
    if (wait(&ws)<0) // grabs info about le child once they exit
        return errno;
    
    return (ws >> 8) & 255; // returns child exit status
}
