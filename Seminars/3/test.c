#include <stdio.h>
#include "green.h"


int counter = 0;
int flag = 0;
green_cond_t cond;
green_mutex_t mutex;

void *testTimer(void *arg){
	int id = *(int*)arg;
	int loop = 40000;
	while(loop > 0){
		if(flag == id){
			printf("thread %d: %d\n", id, loop);
			loop--;
			flag = (id + 1) % 4;
			green_cond_signal(&cond);
		}else{
			green_cond_signal(&cond);
			green_cond_wait(&cond);
		}
	}
}

/**
The program falls short without the signal before the wait.
With the signal on the condition before the wait the program works 
as intended.
**/

void *testStd(void *arg){
	int i = *(int*)arg;
	int loop = 40000;
	while(loop > 0){
		printf("thread %d: %d\n", i, loop);
		loop--;
		green_yield();
	}
}


void* testSharedResource(void* arg) {
  int id = *(int*)arg;
  for(int i = 0; i < 100000; i++) {
    //green_mutex_lock(&mutex);
    int timeWaste = 0;
    while(timeWaste < 1000){
    	timeWaste++;
    }
    counter++;
    //green_mutex_unlock(&mutex);
  }
}

int main(){
	green_t g0;
	green_t g1;
	green_t g2;
	green_t g3;

	int a0 = 0;
	int a1 = 1;
	int a2 = 2;
	int a3 = 3;

	green_mutex_init(&mutex);
	green_cond_init(&cond);
	green_create(&g0, testSharedResource, &a0);
	green_create(&g1, testSharedResource, &a1);
	green_create(&g2, testSharedResource, &a2);
	green_create(&g3, testSharedResource, &a3);

	green_join(&g0);
	printf("Thread nr %d done\n", a0);
	green_join(&g1);
	printf("Thread nr %d done\n", a1);
	green_join(&g2);
	printf("Thread nr %d done\n", a2);
	green_join(&g3);
	printf("Thread nr %d done\n", a3);
	printf("count is %d\n", counter);
	printf("done\n");
	return 0;
}