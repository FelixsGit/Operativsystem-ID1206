#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#define MIN 32
#define LEVELS 8 
#define PAGE 4096

enum flag {Free, Taken};

struct head* freeLists[LEVELS] = {NULL};

//This is the header of each block
struct head{
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;
};

//This function creates a new block
struct head* new() {
  struct head *new = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED) {
    printf("mmap failed: error %d\n", errno);
    return NULL;
  }
  assert(((long int)new & 0xfff) == 0);  // 12 last bits should be zero 
  new->status = Free;
  new->level = LEVELS -1;
  return new;
}

//This function find the buddy of the entered block 
struct head* buddy(struct head* block){
  int index = block->level;
  long int mask = 0x1 << (index + MIN);
  return (struct head*)((long int ) block ^ mask);
}

//This function merge two blocks into one 
struct head* merge(struct head* block, struct head* sibling){
  struct head *primary;
  if(sibling < block){
    primary = sibling;
  }else{
    primary = block;
  }
  primary->level = primary->level + 1;
  return primary;
}

//This function hides the header of the entered block
void *hide(struct head* block){
  return (void*)(block + 1);
}

//This function finds the full block with its header
struct head *magic(void* memory){
  return ((struct head*) memory - 1);
}

//This function find the level of a block given the entered size
int level(int size){
  int req = size + sizeof(struct  head);
  printf("required size of block: = %d byte\n", req);
  if(req > 4096){
    return 8;
  }
  if(req > 2048){
    return 7;
  }
  if(req > 1024){
    return 6;
  }
  if(req > 515){
    return 5;
  }
  if(req > 256){
    return 4;
  }
  if(req > 128){
    return 3;
  }
  if(req > 64){
    return 2;
  }
  if(req > 32){
    return 1;
  }
  if(req > 0){
    return 0;
  }
  else{
    return NULL;
  }

  /*
  printf("req = %lu\n", req);
  int i = 0;
  req = req >> MIN; 
  printf("req = %lu\n", req);
  while(req > 0){
    i++;
    req = req >> 1;
  }
  return i;
  */
}

//This function splits a block and returns the new block
struct head *split(struct head *block) {
  int index = block->level - 1;
  int mask =  0x1 << (index + MIN);
  struct head* newBlock = (struct head*)((long int) block | mask);
  newBlock->level = index;
  printf("a block was moved down to level %d\n",newBlock->level );
  return newBlock;
}

struct head* find(int index){
  printf("Trying to find a free block at level %d, aka a block with size %d byte\n", index, 1*64);
  for(int i = index; i < LEVELS; i++){
    if(freeLists[i] != NULL){
      int timesToSplit = i - index;
      if(timesToSplit == 0){
        printf("Block %p was found at level: %d\n",freeLists[i], i);
        freeLists[i]->status = Taken;
        return freeLists[i];
      }else{
        printf("level %d had free block with address: %p\n",i, freeLists[i]);
        printf("Spliting...\n");
        freeLists[i - 1] = split(freeLists[i]);
        find(index);
        break;
      }
    }
    printf("level %d was empty\n", i);
  }
}

void *balloc(size_t size){
  if(size == 0){
    return NULL;
  }else{
    int index = level(size);
    if(index > 7){
      return NULL;
    }
    struct head* taken = find(index);
    return hide(taken); 
  }
}

//This function is used to test the different functionallity above
void test(){
  struct head* startingMemory = new();
  freeLists[LEVELS - 1] = startingMemory;
  struct head* givenBlock = balloc(12);
  if(givenBlock == NULL){
    printf("Maximum balloc value exceded, you have to request less memory\n");
  }else{
    printf("User received block %p (header removed)\n",givenBlock);
  }
  /*
  struct head *first = new();
  struct head *second = new();
  struct head *third = new();
  first->next = second;
  second->prev = first; 
  second->next = third;
  third->prev = second;

  printf("1st block = %p, with (level = %d, prev = %p, next = %p)\n",first, first->level, first->prev, first->next);
  printf("2st block = %p, with (level = %d, prev = %p, next = %p)\n",second, second->level, second->prev, second->next);
  printf("3st block = %p, with (level = %d, prev = %p, next = %p)\n",third, third->level, third->prev, third->next);
  */

}
