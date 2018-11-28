//#include "buddytest.h"
#include "buddy.h"
#include <stdio.h>

int main(){

	//TODO
	/*
	Beter benchmarks?
	*/

  printf("Welcome to benchmark for a homemade malloc and free implementation\nEnter number of rounds to request:\n");

  int rounds;
  scanf("%d", &rounds);

  int loops;
  printf("Enter maximum amount of pages allowed\n");
  scanf("%d", &loops);

  int max;
  printf("Please chooce as max > min and with a size between 1 and 4072\n");
  printf("Enter maximum block size:\n");
  scanf("%d", &max);

  int min;
  printf("Enter minimum block size:\n");
  scanf("%d", &min);


  test(rounds,loops, max, min);
}
