#include <stdlib.h>
#include <ucontext.h>
#include <stdio.h>
#include <assert.h>
#include "green.h"

#define FALSE 0
#define TRUE 1
#define STACK_SIZE 4096

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, FALSE};
static green_t *running = &main_green;

static void init() __attribute__((constructor));

void addToReadyQueue(green_t* thread);
void tellNextToRun();
void addToQueue(green_t** queue, green_t* threadToAdd);
green_t* popFromQueue(green_t** queue);

//This function is used at the start to initialize the program
void init(){
	getcontext(&main_cntx);
}

void addToReadyQueue(green_t *ready){
	addToQueue(&running, ready);
}
void tellNextToRun(){
	popFromQueue(&running);
}

void addToQueue(green_t **queue, green_t *threadToAdd){
	green_t *current = *queue;
	if(current == NULL){
		*queue = threadToAdd;
	}else{
		while(current->next != NULL){
			current = current->next;
		}
		current->next = threadToAdd;
	}
}
green_t* popFromQueue(green_t **queue){
	green_t *popped = *queue;
	if(popped != NULL){
		*queue = popped->next;
		popped->next = NULL;
	}
	return popped;
}

//This function is the 'main running part of the thread'
void green_thread(){
	//Thread starts its execution
	green_t *this = running;
	(*this->fun)(this->arg);
	//Thread is not done with its execution
	//Place waiting (joining) thread in the ready queue
	if(this->join != NULL){
		addToReadyQueue(this->join);
	}
	//free alocated memory structures
	free(this->context->uc_stack.ss_sp);
	free(this->context);
	//set the status to zombie
	this->zombie = TRUE;
	//find the next thread to run
	tellNextToRun();
	//thread now dead
	setcontext(running->context);
}

//This function is used to create a new green thread 
int green_create(green_t *new, void*(*fun)(void*), void *arg){
	ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
	getcontext(cntx);
	void *stack = malloc(STACK_SIZE);
	cntx->uc_stack.ss_sp = stack;
	cntx->uc_stack.ss_size = STACK_SIZE;
	makecontext(cntx, green_thread, 0);
	new->context = cntx;
	new->fun = fun;
	new->arg = arg;
	new->next = NULL;
	new->join = NULL;
	new->zombie = FALSE;
	addToReadyQueue(new);
	return 0;
}

//This function tells the running thread to wait and then tells the next 
//thread in the free list to running and makes the context switch.
int green_yield(){
	green_t * susp = running;
	//add susp to ready queue
	addToReadyQueue(susp);
	//select the next thread for execution
	tellNextToRun();
	swapcontext(susp->context, running->context);
	return 0;
}

int green_join(green_t *thread){
	if(thread->zombie){
		return 0;
	}
	green_t *susp = running;
	//Add to waiting thread
	if(thread->join == NULL){
		thread->join = susp;
	}else{
		green_t *waitingThread = thread->join;
		while(waitingThread->next != NULL){
			waitingThread = waitingThread->next;
		}
		waitingThread->next = susp;
	}
	//Select the next thread for execution
	tellNextToRun();
	swapcontext(susp->context, running->context);
	return 0;
}


void green_cond_init(struct green_cond_t *cond){
	cond->suspendedThreads = NULL;
}

void green_cond_wait(struct green_cond_t *cond){
	addToQueue(&(cond->suspendedThreads), running);
	green_t *susp = running;
	tellNextToRun();
	swapcontext(susp->context, running->context);
}

void green_cond_signal(struct green_cond_t *cond){
	green_t *threadToSignal = popFromQueue(&cond->suspendedThreads);
	addToReadyQueue(threadToSignal);
}



