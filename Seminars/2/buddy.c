#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
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

void test(){
  printf("new block created with level %d\n", new()->level );
}

int main(){
  test();
}
