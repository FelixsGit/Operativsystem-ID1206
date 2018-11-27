#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>


#define MIN  5     
#define LEVELS 8   
#define PAGE 4096

enum flag {Free, Taken};

struct head {
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;  
};


/* The free lists */
struct head *flists[LEVELS] = {NULL};
int pagesAllocated = 0;
int numberOfFreePages = 0;

//This function appends a block to the flists structure
void appendToflists(struct head* blockToAppend){
  blockToAppend->status = Free;
	if(flists[blockToAppend->level] == NULL){
		blockToAppend->prev = NULL;
		blockToAppend->next = NULL;
		flists[blockToAppend->level] = blockToAppend;
	}else{
		blockToAppend->next = flists[blockToAppend->level];
		blockToAppend->next->prev = blockToAppend;
		flists[blockToAppend->level] = blockToAppend;
	}
}

//This function removes a block from the flists structure
void removeFromflists(int level){
	if(flists[level] == NULL){
		return;
	}else if(flists[level]->next != NULL){
		flists[level]->next->prev = NULL;
		flists[level] = flists[level]->next;
	}else{
		flists[level] = NULL;
	}
}

//This function prints the number of free block on each level in the flists structure
void printflists(){
	printf("--flists current values--\n");
	for(int i = LEVELS - 1; i > - 1; i--){
		int z = 0;
		struct head* test = flists[i];
		if(test != NULL){
			z = 1;
	 		while(test->next != NULL){
			   z++;
			   test = test->next;
			}
		}
		printf("Free blocks at level %d = %d\n",i, z);
	}
}


/* These are the low level bit fidling operations */
struct head *new() {
  struct head *new = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED) {
    printf("mmap failed: error %d\n", errno);
    return NULL;
  }
  assert(((long int)new & 0xfff) == 0);  // 12 last bits should be zero 
  new->level = LEVELS -1;
  new->status = Free;
  pagesAllocated++;
  return new;
}

struct head *buddy(struct head* block) {
  int index = block->level;
  long int mask =  0x1 << (index + MIN);
  return (struct head*)((long int)block ^ mask);
}

struct head *primary(struct head* block) {
  int index = block->level;
  long int mask =  0xffffffffffffffff << (1 + index + MIN);
  return (struct head*)((long int)block & mask);
}

struct head *split(struct head *block) {
  block->status = Taken;
  int index = block->level - 1;
  int mask =  0x1 << (index + MIN);
  struct head* rightNewBlock = (struct head *)((long int)block | mask);
  struct head* leftNewBlock = block;
  rightNewBlock->level = block->level - 1;
  leftNewBlock->level = block->level - 1;
  removeFromflists(block->level + 1);
  appendToflists(leftNewBlock);
  appendToflists(rightNewBlock);
}


void *hide(struct head* block) {
  return (void*)(block + 1);
}

struct head *magic(void *memory) {
  return (struct head*)(((struct head*)memory) - 1);
}

int level(int req) {
  int total = req  + sizeof(struct head);
  int i = 0;
  int size = 1 << MIN;
  while(total > size) {
    size <<= 1;
    i += 1;
  }
  return i;
}


struct head *find(int index, short int level) {
 	if(flists[index] == NULL){
 		if(index > 6){
 			appendToflists(new());
 			find(index, level);
 		}else{
 			find(index+ 1, level);
 		}
 	}else if(index != level){
 		split(flists[index]); 
 		find(index - 1, level);
 	}else{
 		struct head* blockToGive = flists[index];
    blockToGive->status = Taken;
 		removeFromflists(index);
 		blockToGive->level = index;
 		return blockToGive;
 	}
}


void *balloc(size_t size) {
  if( size == 0 ){
    return NULL;
  }
  int index = level(size);
  struct head *taken = find(index, index);
  if(taken == NULL){
  	return NULL;
  }
  return hide(taken);
}


void insert(struct head* block) {
  block->status = Free;
  if(block->level > 6){
    appendToflists(block);
    numberOfFreePages++;
    //TODO: a page is totaly free and should be dropped here
  }else{
    struct head* testBuddy = buddy(block);
    if((testBuddy->status == Free) && (block->level == testBuddy->level)){
      struct head* coalescedBlock = primary(testBuddy);
      removeFromflists(block->level);
      removeFromflists(block->level);
      coalescedBlock->level = block->level + 1;
      insert(coalescedBlock);
    }else{
      appendToflists(block);
    }
  }
  return;
}

void bfree(void *memory) {
	//printf("freeing block\n");
  if(memory != NULL) {
    struct head *block = magic(memory);
    //printf("trying to free block %p at level = %d\n", block, block->level);
    insert(block);
  }else{
  	//printf("cant free an empty block, returning...\n");
  }
}

int randr(unsigned int min, unsigned int max){
       double scaled = (double)rand()/RAND_MAX;

       return (max - min +1)*scaled + min;
}

// Test sequences
void test(int numberOfBlocks, int maxBlockSize, int minBlockSize, int maximumPagesAllowed) { 
	appendToflists(new());
  clock_t start, end;
  double cpu_time_used;
	srand(time(NULL));
	start = clock();
  	for(int j = 0; j < numberOfBlocks; j++){
  		int *memory;
  		int size = randr(minBlockSize, maxBlockSize);
     	memory = balloc(size);
      *memory = 20;
     	if(memory == NULL){
      	fprintf(stderr, "malloc failed\n");
        return;
      }
      if(pagesAllocated >= maximumPagesAllowed){
  			bfree(memory);
  			pagesAllocated--;
  		}	
	}
  printf("Free complete\n");
  printflists();
	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("cpu time used = %f\n",cpu_time_used );
}
