#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#define MIN 32
#define LEVELS 8
#define PAGE 4096

enum flag {Free, Taken};

//This is the header of each block
struct head{
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;
};

//This function creates a new block
struct head* new(){
  struct head *new = (struct head*) mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED){
    return NULL;
  }
  assert(((long int) new & 0xfff) == 0);
  new->status = Free;
  new->level = LEVELS - 1;
  return new;
};

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
  int i = 0;
  req = req >> MIN; 
  while(req > 0){
    i++;
    req = req >> 1;
  }
  return i;
}

//This function splits a block and returns the new block
struct head* split(struct head* block, int index){
  long int mask = 0x1 << (index + MIN);
    return (struct head*)((long int) block | mask);
}

//This function is used to test the different functionallity above
void test(){
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

}
