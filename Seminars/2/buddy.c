#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#define MIN 32
#define LEVELS 8 
#define PAGE 4096

enum flag {Free, Taken, Split};

struct head* memory;

//This is the header of each block
struct head{
  enum flag status;
  short int level;
  int size;
  struct head *leftChildBlock;
  struct head *rightChildBlock;
};

//This function creates a new block
void newMemory() {
  memory = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(memory == MAP_FAILED) {
    printf("mmap failed: error %d\n", errno);
  }else{
    assert(((long int)memory & 0xfff) == 0);  // 12 last bits should be zero 
    memory->status = Free;
    memory->level = LEVELS -1;
    memory->size = PAGE;
  }
}
struct head* newBlock(int size, short int level) {
   struct head* newBlock = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(newBlock == MAP_FAILED) {
    printf("mmap failed: error %d\n", errno);
  }else{
    assert(((long int)newBlock & 0xfff) == 0);  // 12 last bits should be zero 
    newBlock->status = Free;
    newBlock->level = level;
    newBlock->size = size;
    return newBlock;
  }
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
  //printf("required size of block: = %d byte aka level", req);
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
}

//This function splits a block and returns the new block
void split(struct head* parentBlock) {
  int index = parentBlock->level - 1;
  int mask =  0x1 << (index + MIN);
  struct head* newLeftBlock = newBlock(parentBlock->size/2, index);
  struct head* newRightBlock = newBlock(parentBlock->size/2, index);
  parentBlock->status = Split;
  newRightBlock->status = Free;
  newLeftBlock->status = Free;

  newRightBlock->level = index;
  newLeftBlock->level = index;

  parentBlock->rightChildBlock = newRightBlock;
  parentBlock->leftChildBlock = newLeftBlock;
}

//This function returns a block of the size declared by the entered index
struct head* find(struct head* currentBlock, int index){
  //printf("Arrived at block %p with level = %d, status = %d size = %d\n",currentBlock, currentBlock->level, currentBlock->status, currentBlock->size);
  if(currentBlock->level == index){
    if(currentBlock->status == Free){
      //printf("BLOCK TAKEN\n");
      currentBlock->status = Taken;
      return currentBlock;
    }else if(currentBlock->level < 7){
      //printf("going back\n");
      return NULL;
    }
    else{
      if(currentBlock->level == 7){
        //printf("MEMORY FULL NO MEMORY AVAILABLE\n");
      }
      return NULL;
    }
  }else{
    if(currentBlock->status == Split){
      //printf("going left...\n");
      if(find(currentBlock->leftChildBlock, index) == NULL){
        //printf("Arrived at block %p with level = %d, status = %d size = %d\n",currentBlock, currentBlock->level, currentBlock->status, currentBlock->size);
        //printf("going right...\n");
        if(find(currentBlock->rightChildBlock, index) == NULL){
          if(currentBlock->level == 7){
            //printf("MEMORY FULL NO MEMORY AVAILABLE\n");
          }
          return NULL;
        }
      }
    }else if(currentBlock->status == Taken){
      if(currentBlock->level == 7){
        //printf("MEMORY FULL NO MEMORY AVAILABLE\n");
      }
      return NULL;
    }else if(currentBlock->status == Free){
      //printf("spliting block\n");
      split(currentBlock);
      find(currentBlock, index);
    }
  }
}

//This function is used to allocate new memory of the entered size
void *balloc(size_t size){
  if(size == 0){
    return NULL;
  }else{
    int index = level(size);
    if(index > 7){
      return NULL;
    }
    //printf(" %d\n", index);
    struct head* taken = find(memory, index);
    if(taken == NULL){
      return NULL;
    }
    return hide(taken); 
  }
}

int _print_t(struct head* tree, int is_left, int offset, int depth, char s[20][255]){
    char b[20];
    int width = 5;
    if (!tree) return 0;
    sprintf(b, "(%03d)", tree->status);
    int left  = _print_t(tree->leftChildBlock,  1, offset,                depth + 1, s);
    int right = _print_t(tree->rightChildBlock, 0, offset + left + width, depth + 1, s);
#ifdef COMPACT
    for (int i = 0; i < width; i++)
        s[depth][offset + left + i] = b[i];
    if (depth && is_left) {
        for (int i = 0; i < width + right; i++)
            s[depth - 1][offset + left + width/2 + i] = '-';
        s[depth - 1][offset + left + width/2] = '.';
    } else if (depth && !is_left) {
        for (int i = 0; i < left + width; i++)
            s[depth - 1][offset - width/2 + i] = '-';
        s[depth - 1][offset + left + width/2] = '.';
    }
#else
    for (int i = 0; i < width; i++)
        s[2 * depth][offset + left + i] = b[i];
    if (depth && is_left) {
        for (int i = 0; i < width + right; i++)
            s[2 * depth - 1][offset + left + width/2 + i] = '-';
        s[2 * depth - 1][offset + left + width/2] = '+';
        s[2 * depth - 1][offset + left + width + right + width/2] = '+';
    } else if (depth && !is_left) {
        for (int i = 0; i < left + width; i++)
            s[2 * depth - 1][offset - width/2 + i] = '-';
        s[2 * depth - 1][offset + left + width/2] = '+';
        s[2 * depth - 1][offset - width/2 - 1] = '+';
    }
#endif
    return left + width + right;
}

void print_t(struct head* tree){
    char s[20][255];
    for (int i = 0; i < 20; i++)
        sprintf(s[i], "%80s", " ");
    _print_t(tree, 0, 0, 0, s);
    for (int i = 0; i < 20; i++)
        printf("%s\n", s[i]);
}

//This function is used to test the different functionallity above
void test(){
  newMemory();

  struct head* givenBlockOne = balloc(768);
  if(givenBlockOne == NULL){
   printf("Memory could not be allocated\n\n");    
  }else{      
    printf("User received block %p (header removed)\n\n",givenBlockOne);
  }

  struct head* givenBlockTwo = balloc(1200);
  if(givenBlockTwo == NULL){
   printf("Memory could not be allocated\n\n");    
  }else{      
    printf("User received block %p (header removed)\n\n",givenBlockTwo);
  }

  struct head* givenBlockThree = balloc(32);
  if(givenBlockThree == NULL){
   printf("Memory could not be allocated\n\n");    
  }else{      
    printf("User received block %p (header removed)\n\n",givenBlockThree);
  }
  
  print_t(memory);
}
