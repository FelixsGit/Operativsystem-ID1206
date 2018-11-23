#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(){

  int pid = fork();

  if(pid == 0){
    printf("check the status\n");
    sleep(10);
  }else{
    int res;
    wait(&res);
    printf("the result was %d\n", WEXITSTATUS(res));
  }
  return 0;
}
