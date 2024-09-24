#include<signal.h>
#include<stdlib.h>
#include<stdio.h>

void handle_sigstp(int sig){
    printf("Process stopped\n");
}
//to handle ctrl+c
void handle_second(int sig){
    printf("Process interuppted");
}

int main(int argc,char* argv[]){
    struct sigaction z;
    z.sa_handler=&handle_sigstp;
    z.sa_flags=SA_RESTART;
    sigaction(SIGTSTP,&z,NULL);

// no need to set the flags again it remains the same when set once
    z.sa_handler=&handle_second;
    sigaction(SIGINT,&z,NULL);
    return 0;


}
