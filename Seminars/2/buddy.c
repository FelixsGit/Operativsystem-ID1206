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

#define BUFFER 1010000

enum flag {Free, Taken};

struct head {
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;  
};

/* The free lists */
struct head* flists[LEVELS] = {NULL};
int pagesAllocated = 0;
long int totalInternalFrag = 0;
long int totalAmountAllocated = 0;
long int totalMemoryGiven = 0;

//This function appends a block to the flists structure
void appendToflists(struct head* blockToAppend){
  blockToAppend->status = Free;
	blockToAppend->next = flists[blockToAppend->level];
  blockToAppend->prev = NULL;
  flists[blockToAppend->level] = blockToAppend;
  if(blockToAppend->next != NULL){
    blockToAppend->next->prev = blockToAppend;
  }
}

//This function removes a block from the flists structure
void removeFromflists(struct head* block){
  //block->status = Taken;
  if(block->next == NULL && block->prev == NULL){
    flists[block->level] = NULL;
  }else if(block->prev != NULL && block->next == NULL){
    block->prev->next = NULL;
  }else if(block->prev == NULL && block->next != NULL){
    block->next->prev = block->prev;
    flists[block->level] = block->next;
  }else{
    block->prev->next = block->next;
    block->next->prev = block->prev;
  }
  block->next = NULL;
  block->prev = NULL;
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
  //printf("Heap is pointing to %p\n", new);
  totalMemoryGiven += PAGE;
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
  removeFromflists(block);
  block->status = Taken;
  int index = block->level - 1;
  int mask =  0x1 << (index + MIN);
  struct head* rightNewBlock = (struct head *)((long int)block | mask);
  struct head* leftNewBlock = block;
  rightNewBlock->level = block->level - 1;
  leftNewBlock->level = block->level - 1;
  appendToflists(rightNewBlock);
  appendToflists(leftNewBlock);
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

int getSizeFromLevel(int level){
   if(level == 0){
    return 32;
  }else if(level == 1){
    return 64;
  }else if(level == 2){
    return 128;
  }else if(level == 3){
    return 256;
  }else if(level == 4){
    return 512;
  }else if(level == 5){
    return 1024;
  }else if(level == 6){
    return 2048;
  }else if(level == 7){
    return 4096;
  }
}

void updateCounters(int level, long int sizeOfBlockNeeded){
  totalInternalFrag += (getSizeFromLevel(level) - sizeOfBlockNeeded);
  totalAmountAllocated += getSizeFromLevel(level);
}

struct head *find(int index, short int level, int sizeOfBlockNeeded) {
 	if(flists[index] == NULL){
 		if(index > 6){
 			appendToflists(new());
 			find(index, level, sizeOfBlockNeeded);
 		}else{
 			find(index+ 1, level, sizeOfBlockNeeded);
 		}
 	}else if(index != level){
 		split(flists[index]); 
 		find(index - 1, level, sizeOfBlockNeeded);
 	}else{
 		struct head* blockToGive = flists[index];
    blockToGive->status = Taken;
    blockToGive->level = index;
 		removeFromflists(blockToGive);
    updateCounters(level, sizeOfBlockNeeded);
 		return blockToGive;
 	}
}


void *balloc(size_t size) {
  if(size == 0){
    return NULL;
  }
  int index = level(size);
  int totalsize = size + sizeof(struct head);
  struct head *taken = find(index, index, totalsize);
  if(taken == NULL){
  	return NULL;
  }
  return hide(taken);
}


void insert(struct head* block) {
  block->status = Free;
  if(block->level > 6){
    block->status = Free;
    appendToflists(block);
    //printf("munmap active\n");
    //munmap(block, PAGE);
  }else{
    struct head* testBuddy = buddy(block);
    if((testBuddy->status == Free) && (block->level == testBuddy->level)){
      removeFromflists(testBuddy);
      struct head* coalescedBlock = primary(testBuddy);
      coalescedBlock->level = block->level + 1;
      insert(coalescedBlock);
    }else{
      appendToflists(block);
    }
  }
  return;
}

void bfree(void *memory) {
  if(memory != NULL) {
    struct head *block = magic(memory);
    insert(block);
  }else{
  	//printf("cant free an empty block, returning...\n");
  }
}

int getExternalFrag(){
  int externalFrag = 0;
  for(int i = 0; i < LEVELS - 1; i++){
    struct head* freeBlock = flists[i];
    while(freeBlock != NULL){
      externalFrag += getSizeFromLevel(i);
      freeBlock = freeBlock->next;
    }
  }
  return externalFrag;
}

int randr(unsigned int min, unsigned int max){
       double scaled = (double)rand()/RAND_MAX;

       return (max - min +1)*scaled + min;
}

// Test sequences
void test(int rounds, int loops, int maxBlockSize, int minBlockSize) { 
  void *buffer[BUFFER];
  for(int i =0; i < BUFFER; i++) {
    buffer[i] = NULL;
  }

  clock_t start, end;
  double cpu_time_used;
  srand(time(NULL));
  start = clock();

  for(int j = 0; j < rounds; j++) {
    for(int i= 0; i < loops ; i++) {
      int index = rand() % BUFFER;
      if(buffer[index] != NULL) {
        bfree(buffer[index]);
      }
      size_t size = (size_t)randr(minBlockSize,maxBlockSize);
      int* memory; 
      memory = balloc(size);

      if(memory == NULL) {
        memory = balloc(0); 
        fprintf(stderr, "memory myllocation failed, last address %p\n", memory);
        return;
      }
      buffer[index] = memory;
      *memory = 122;/* writing to the memory so we know it exists */
    }
  }
  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("cpu time used = %f\n",cpu_time_used );

  int extFrag = getExternalFrag();
  long double procentIntFrag = (((long double)(totalInternalFrag)/((long double)totalAmountAllocated))*100);
  long double procentExFrag = (((long double)(extFrag)/((long double)totalMemoryGiven))*100);
  printf("total internal fragmentation is = %ld/%ld bytes, -> %Lf%% \n",totalInternalFrag, totalAmountAllocated, procentIntFrag);
  printf("total external fragmentation is = %d/%ld bytes, -> %Lf%% \n",extFrag, totalMemoryGiven, procentExFrag);
  printflists();
}