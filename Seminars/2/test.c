#include "buddy.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>  //-lm
#include <time.h>

#define BUFFER 1000
#define LOOPS 100000
#define MIN 8
#define MAX 4000

// Request smaller blocks far more often so simulate real behaviour of user
// size = MAX / e^r, where e^r [1, MAX/MIN] => size [MIN, MAX]
int request() {
  if(MIN == MAX)
    return MIN;
  double k = log(((double) MAX) / MIN);
  double r = ((double)(rand() % (int)(k*10000))) / 10000;
  int size = (int)((double) MAX / exp(r));

  return size;
}

void benchMemory() {
  void *buffer[BUFFER]; 
  for(int i = 0; i < BUFFER; i++) {
    buffer[i] = NULL;
  }
  double start = clock();
  for(int i = 0; i < LOOPS; i++) {
    int index = rand() % BUFFER;
    if(buffer[index] != NULL){
      bfree(buffer[index]);
    }
    size_t size = (size_t)request();
    int* memory;
    memory = balloc(size);
    if(memory == NULL) {
      fprintf(stderr, "mem alloc failed\n");
      return;
    }
    buffer[index] = memory;
    *memory = 123;

    if(i % 1000 == 0){
      writeFrag((double)((clock() - start)/CLOCKS_PER_SEC));
    }
  }
}

void benchTime(){
  int size_max = 4070;
  int numOps = 10000;
  int stride = 10;
  // int ROUNDS = 1;
  clock_t c_startBalloc, c_stopBalloc, c_startBfree, c_stopBfree, c_startFree, c_stopFree, c_startMalloc, c_stopMalloc;
  printf("# Evaluation of time performance \n#\n#{AllocSize(Byte)\tTime(ms)Bfree\tTime(ms)Free}\n");
  double processTimeBalloc = 0, processTimeMalloc, processTimeBfree = 0, processTimeFree = 0;
  for(int size = stride; size <= size_max; size+=stride) {

    void* buffer[numOps]; 

    //TESTING MALLOC 
    c_startMalloc = clock();
    for(int i = 0; i < numOps; i++) {
      buffer[i] = malloc(size);
    }
    c_stopMalloc = clock();
    processTimeMalloc = ((double)(c_stopMalloc - c_startMalloc)) / ((double)CLOCKS_PER_SEC/1000);

    //freeing elements
    for(int i = 0; i < numOps; i++) {
      free(buffer[i]);
    }
  
    //TESTING BALLOC
    c_startBalloc = clock();
    for(int i = 0; i < numOps; i++) {
      buffer[i] = balloc(size);
    }
    c_stopBalloc = clock();
    processTimeBalloc = ((double)(c_stopBalloc - c_startBalloc)) / ((double)CLOCKS_PER_SEC/1000);
    

    //TESTING BFREE
    c_startBfree = clock();
    for(int i = 0; i < numOps; i++) {
      bfree(buffer[i]);
    }
    c_stopBfree = clock();
    processTimeBfree = ((double)(c_stopBfree - c_startBfree)) / ((double)CLOCKS_PER_SEC/1000);

    //adding new elements 
    for(int i = 0; i < numOps; i++) {
      buffer[i] = malloc(size);
    }

    //TESTING FREE
    c_startFree = clock();
    for(int i = 0; i < numOps; i++) {
      free(buffer[i]);
    }
    c_stopFree = clock();
    processTimeFree = ((double)(c_stopFree - c_startFree)) / ((double)CLOCKS_PER_SEC/1000);
    

    double avgOpTimeMalloc = (processTimeMalloc)/numOps;
    double avgOpTimeBalloc = (processTimeBalloc)/numOps;
    double avgOpTimeFree = (processTimeFree)/numOps;
    double avgOpTimeBfree = (processTimeBfree)/numOps;
    printf("%d\t%f\t%f\n", size, avgOpTimeBfree, avgOpTimeFree);
    printf("%d\t%f\t%f\n", size, avgOpTimeBalloc, avgOpTimeMalloc);
  }
}

int main(){

  srand(time(NULL));

  //benchMemory();

  benchTime();


	
}
