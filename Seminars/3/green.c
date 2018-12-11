#include <stdlib.h>
#include <ucontext.h>
#include <stdio.h>
#include <assert.h>
#include "green.h"
#include <signal.h>
#include <sys/time.h>

#define PERIOD 100
#define FALSE 0
#define TRUE 1
#define STACK_SIZE 4096

static sigset_t block;
static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, FALSE};
static green_t *running = &main_green;

static void init() __attribute__((constructor));

void timer_handler(int);
void addToReadyQueue(green_t* thread);
void tellNextToRun();
void addToQueue(green_t** queue, green_t* threadToAdd);
green_t* popFromQueue(green_t** queue);

//This function is used at the start to initialize the program
void init(){
	//getcontext(&main_cntx);
	sigemptyset(&block);
	sigaddset(&block, SIGVTALRM);
	struct sigaction act = {0};
	struct timeval interval;
	struct itimerval period;
	act.sa_handler = timer_handler;
	assert(sigaction(SIGVTALRM, &act, NULL) == 0);
	interval.tv_sec = 0;
	interval.tv_usec = PERIOD;
	period.it_interval = interval;
	period.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &period, NULL);
}

int green_mutex_init(green_mutex_t *mutex){
	mutex->taken = FALSE;
	mutex->susp = NULL;
}

int green_mutex_lock(green_mutex_t *mutex){
	sigprocmask(SIG_BLOCK, &block, NULL);
	green_t *susp = running;
	while(mutex->taken){
		//suspend the running thread
		addToQueue(&(mutex->susp), susp);
		//tell next thread to run
		tellNextToRun();
		swapcontext(susp->context, running->context);
	}
	mutex->taken = TRUE;
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return 0;
}

int green_mutex_unlock(green_mutex_t *mutex){
	sigprocmask(SIG_BLOCK, &block, NULL);
	//move suspended threads to ready queue;
	addToReadyQueue(mutex->susp);
	mutex->susp = NULL;
	mutex->taken = FALSE;
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return 0; 
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
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	//Thread starts its execution
	green_t *this = running;
	(*this->fun)(this->arg);
	//Thread is not done with its execution
	//Place waiting (joining) thread in the ready queue
	sigprocmask(SIG_BLOCK, &block, NULL);
	if(this->join != NULL){
		addToReadyQueue(this->join);
	}
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	//free alocated memory structures
	free(this->context->uc_stack.ss_sp);
	free(this->context);
	//set the status to zombie
	this->zombie = TRUE;
	//find the next thread to run
	sigprocmask(SIG_BLOCK, &block, NULL);
	tellNextToRun();
	sigprocmask(SIG_UNBLOCK, &block, NULL);
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
	sigprocmask(SIG_BLOCK, &block, NULL);
	addToReadyQueue(new);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return 0;
}

//This function tells the running thread to wait and then tells the next 
//thread in the free list to running and makes the context switch.
int green_yield(){
	sigprocmask(SIG_BLOCK, &block, NULL);
	green_t * susp = running;
	//add susp to ready queue
	addToReadyQueue(susp);
	//select the next thread for execution
	tellNextToRun();
	swapcontext(susp->context, running->context);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return 0;
}

int green_join(green_t *thread){
	if(thread->zombie){
		return 0;
	}
	green_t *susp = running;
	sigprocmask(SIG_BLOCK, &block, NULL);
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
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return 0;
}

void timer_handler(int sig){
	//printf("timmer triggered\n");
	green_yield();
}

void green_cond_init(struct green_cond_t *cond){
	cond->suspendedThreads = NULL;
}

/*
void green_cond_wait(struct green_cond_t *cond){
	sigprocmask(SIG_BLOCK, &block, NULL);
	addToQueue(&(cond->suspendedThreads), running);
	green_t *susp = running;
	tellNextToRun();
	swapcontext(susp->context, running->context);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
}
*/
int green_cond_wait(struct green_cond_t *cond, green_mutex_t *mutex){
	sigprocmask(SIG_BLOCK, &block, NULL);
	//suspend the running thread on condition
	green_t *susp = running;
	addToQueue(&(cond->suspendedThreads), susp);
	if(mutex != NULL){
		//release the lock if we have a mutex
		mutex->taken = FALSE;

		//schedule suspended threads
		addToReadyQueue(mutex->susp);
		mutex->susp = NULL;
	}
	//schedule the next thread
	tellNextToRun();
	swapcontext(susp->context, running->context);
	if(mutex != NULL){
		//try to take the lock
		while(mutex->taken){
			//bad luck, suspend the thread;
			green_t *susp = running;
			addToQueue(&(mutex->susp), susp);
			tellNextToRun();
			swapcontext(susp->context, running->context);
		}
		//take the lock
		mutex->taken = TRUE;
	}
	//unblock
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return 0;
}

void green_cond_signal(struct green_cond_t *cond){
	sigprocmask(SIG_BLOCK, &block, NULL);
	green_t *threadToSignal = popFromQueue(&cond->suspendedThreads);
	addToReadyQueue(threadToSignal);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
}




