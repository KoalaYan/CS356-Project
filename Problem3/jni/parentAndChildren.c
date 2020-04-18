#include<stdio.h>
#include<stdlib.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<string.h>


int main(){
    //printf("debugger0.\n");
    pid_t children_pid = fork();
    //printf("debugger1.\n");
    if(children_pid < 0){
        //printf("debugger2.\n");
        printf("Fail to fork.\n");
    }
    else if(children_pid == 0){
        //printf("debugger3.\n");
        pid_t pid = getpid();
        printf("518030910094 Children %d.\n",pid);
        execl("/data/misc/ptreeARM","ptreeARM",NULL);
    }
    else{
        //printf("debugger4.\n");
        pid_t pid = getpid();
        printf("518030910094 Parent%d.\n",pid);
    }

    return 0;
}