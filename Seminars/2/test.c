//#include "buddytest.h"
#include "buddy.h"
#include <stdio.h>

int main(){

	//TODO
	/*
	Add a min value of the requested blocks.

	Add a number to decide how many pages the process sould be able to obtain at a given time.
	meaning when the maximum amount of blocks obtained is reached the process will start to free memory.
	*/

  printf("Welcome to benchmark for a homemade malloc and free implementation\nEnter number of blocks to request:\n");

  int numberOfBlocks;
  scanf("%d", &numberOfBlocks);

  int max;
  printf("Enter maximum block size:\n");
  scanf("%d", &max);

  int min;
  printf("Enter minimum block size:\n");
  scanf("%d", &min);

  int pagesAllowed;
  printf("Enter maximum amount of pages allowed\n");
  scanf("%d", &pagesAllowed);


  test(numberOfBlocks, max, min, pagesAllowed);
}
