#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(){

  int pid = fork();

  if(pid == 0){
    return 42;
  }else{
    int res;
    wait(&res);
    printf("the result was %d\n", WEXITSTATUS(res));
  }
  return 0;
}
