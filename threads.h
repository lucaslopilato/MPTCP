#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include "utility.h"
#include "queue.h"
#include "mptcp.h"

struct thread_in{
	int numsockets;
	int* ports;
	char* server;
	int timeout;
	int verbose;
};

struct function_in{
	int pid; //Pair ID starts at 1
	int socket;
	int port;
	pthread_mutex_t* lock;
	char* server;
	int timeout; //Timeout (sec) before packet retransmission
	int verbose;
};

//Setup and Teardown
void initThreads();
void spawnThreads(pthread_t *threads, int size, struct function_in* input,int inputSize);
void gatherThreads(pthread_t *threads, int size);

//Send and Receive Functions
void* thread_receive(void* input);
void* thread_send(void* input);

//Utility Functions
void threadError(int function);
void initWindow(struct queue_item** window, int size);

#endif //threads.h