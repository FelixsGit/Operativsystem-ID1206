#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>


#define MIN  5     
#define LEVELS 8   
#define PAGE 4096


struct head {
  short int level;
  struct head *next;
  struct head *prev;  
};


/* The free lists */
struct head *flists[LEVELS] = {NULL};

//This function appends a block to the flists structure
void appendToflists(struct head* blockToAppend){
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
  //new->status = Free;
  new->level = LEVELS -1;
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
 	//printf("looking for a block at level %d\n",index);
 	if(flists[index] == NULL){
 		if(index > 6){
 			return NULL;
 		}
 		//printf("no free block found at level %d, going up...\n", index);
 		find(index+ 1, level);
 	}else if(index != level){
 		//printf("block was found! Spliting block and going down to level %d \n", index - 1);
 		split(flists[index]); 
 		find(index - 1, level);
 	}else{
 		struct head* blockToGive = flists[index];
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
  printf("looking to insert the block %p at level %d into the flists\n", block, block->level);
  appendToflists(block);
  if(block->level > 6){
  	printf("reached root, no mory buddies available, returning...\n");
  	printf("\n");
  	printflists();
  	printf("\n");
  	return;
  }else{
  	printf("free block is now inserted, looking for a buddy...\n");
  	printf("\n");
  	printflists();
  	printf("\n");
  	if(flists[block->level]->next != NULL){
  		struct head* currentBuddy =  buddy(block);
  		printf("Buddy found at level %d with address %p, trying to coalesce\n",currentBuddy->level, currentBuddy);
  		struct head* coalescedBlock = primary(block);
  		coalescedBlock->level = block->level + 1;
  		removeFromflists(coalescedBlock->level - 1);
  		removeFromflists(coalescedBlock->level - 1);
  		printf("Coalescense complete, block with address %p added to level %d in the flists\n",coalescedBlock, coalescedBlock->level);
  		insert(coalescedBlock);
  		return;

  	}
  	printf("no buddy found, returning...\n");
  	return;
  }
  return;
}

void bfree(void *memory) {
  if(memory != NULL) {
    struct head *block = magic(memory);
    insert(block);
  }else{
  	printf("cant free an empty block, returning...\n");
  }
}


// Test sequences
void test() {
	appendToflists(new());
	int numberOfBallocs = 16;
	srand(time(NULL));
	/*
	for(int i = 0; i < numberOfBallocs; i++){
		int r = (rand() % (PAGE)/10) +1;
		printf("Trying to request memory of size %d bytes\n", r); 
		struct head* givenBlock = balloc(r);
		if(givenBlock == NULL){
			printf("Error! The requested memory (%d)cant be given, true a lower amount!\n", r);
		}else{
			printf("Program was given block %p\n",givenBlock);
		}
	}
	*/
	//int r = (rand() % (PAGE)/10) +1;
	int r = 870;
	printf("Trying to request memory of size %d bytes\n", r); 
	struct head* givenBlock = balloc(r);
	if(givenBlock == NULL){
		printf("Error! The requested memory(%d bytes) cant be given, try a lower amount!\n", r);
	}else{
		printf("Program was given block %p\n",givenBlock);
	}
	printf("\n");
	r = 1;
	printf("Trying to request memory of size %d bytes\n", r); 
	struct head* givenBlockTwo = balloc(r);
	if(givenBlockTwo == NULL){
		printf("Error! The requested memory (%d bytes) cant be given, try a lower amount!\n", r);
	}else{
		printf("Program was given block %p\n",givenBlockTwo);
	}
	printf("\n");
	printflists();
	printf("\n");
	printf("Trying to free the block %p\n",givenBlockTwo);
	bfree(givenBlockTwo);
	givenBlockTwo = NULL;
	printf("Trying to free the block %p\n",givenBlock);
	bfree(givenBlock);
	givenBlock = NULL;
}
