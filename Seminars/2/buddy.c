#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#define MIN 32
#define LEVELS 8
#define PAGE 4096

enum flag {Free, Taken};

struct head{
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;
};
struct head *new(){
  //here we allocate a full 4096 byte
  struct head *new = (struct head *) mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED){
    return NULL;
  }
  assert(((long int) new & 0xfff) == 0); //last 12 bits should be zero
  new->status = Free;
  new->level = LEVELS - 1;
  return new;
};
struct head *buddy(struct head* block){
  int index = block->level;
  long int mask = 0x1 << (index + MIN);
  return (struct head*)((long int ) block ^ mask);
}
struct head *merge(struct head* block, struct head* sibling){
  struct head *primary;
  if(sibling < block){
    primary = sibling;
  }else{
    primary = block;
  }
  primary->level = primary->level + 1;
  return primary;
}
void *hide(struct head* block){
  return (void*)(block + 1);
}
struct head *magic(void* memory){
  return ((struct head*) memory - 1);
}
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

struct head *split(struct head *block, int index){
  long int mask = 0x1 << (index + MIN);
    return (struct head*)((long int) block | mask);
}

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


  struct head *left = split(first, 7);
  void* p = hide(left);
  void* r = magic(p);
  printf("left = %p, p = %p, r = %p", left, p, r );
  /*
  printf("block %p splited into %p with level = %d\n",first, left, left->level);
  struct head *leftLeft = split(left, 6);
  struct head *lmerged = merge(left,leftLeft);
  struct head *firstRestored = merge(first,lmerged);

  printf("block = %p, with (level = %d, prev = %p, next = %p)\n",firstRestored, firstRestored->level, firstRestored->prev, firstRestored->next);
  */




}
