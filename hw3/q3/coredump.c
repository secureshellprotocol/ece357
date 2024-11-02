// test file, not part of program
#include <signal.h>
int main(){
    raise(SIGSEGV);
}
