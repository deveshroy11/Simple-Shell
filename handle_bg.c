#include<signal.h>
#include<stdlib.h>
#include<stdio.h>

void handle_sigstp(int sig){
    printf("Process stopped\n");
}

int main(int argc,char* argv[]){
    struct sigaction z;
    z.sa_handler=&handle_sigstp;
    z.sa_flags=SA_RESTART;
    sigaction(SIGTSTP,&z,NULL);

    return 0;


}
