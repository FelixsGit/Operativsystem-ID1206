#include <stdio.h>
#include "green.h"


//test 2
int flag = 0;
green_cond_t cond;
void *test2(void *arg){
	int id = *(int*)arg;
	int loop = 4;
	while(loop > 0){
		if(flag == id){
			loop--;
			flag = (id + 1) % 2;
			green_cond_signal(&cond);
		}else{
			green_cond_wait(&cond);
		}
	}
}

/*
//test 1
void *test(void *arg){
	int i = *(int*)arg;
	int loop = 4;
	while(loop > 0){
		printf("thread %d: %d\n", i, loop);
		loop--;
		green_yield();
	}
}
*/
int main(){
	green_t g0;
	green_t g1;

	int a0 = 0;
	int a1 = 1;

	green_cond_init(&cond);
	green_create(&g0, test2, &a0);
	green_create(&g1, test2, &a1);

	green_join(&g0);
	printf("Thread nr %d done\n", a0);
	green_join(&g1);
	printf("Thread nr %d done\n", a1);
	printf("done\n");
	return 0;
}