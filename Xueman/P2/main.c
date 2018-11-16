/* 
  * CSci5103 Fall 2018
  * Assignment 4
  * Group Memebers: Xueman Liang, Yang Yang
  * Student IDs: 4271136, 5305584
  * x500: liang195, yang5276
  * CSELABS Machine: UMN VOLE CSE-IT (http://vole.cse.umn.edu/) Virtual Linux Machine
*/

#include "commons.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	if( argc != 2 ) {
		printf("To execute the program, use command: ./prodcons (int)[buffernSize]\n");
		exit(0);
	}

	// read buffer size from user input
	int bufferSize = atoi(argv[1]);
	if(bufferSize <= 0) {
		printf("Please enter a POSITIVE integer to initialize the buffer.\n");
		exit(0);
	}

	int shmem_id;
	struct sharedContent* shmem_ptr;
	key_t key = 4456;  // a random key to access shared memory
	// the shared memory size should be the size of the sharedContent + the shared buffer size
	int size = sizeof(struct sharedContent) + sizeof(struct item) * bufferSize;
	int flag = 1023;  // all permissions and modes are set

	shmem_id = shmget(key, size, flag);
	if(shmem_id == -1) {
		printf("Failed to shmget in main process.\n");
		exit(1);
	}

	shmem_ptr = shmat(shmem_id, (void*)NULL, flag);
	if(shmem_ptr == (void*)-1) {
		printf("Failed to shmat in main process.\n");
		exit(2);
	}
	// set parameters in shared memory 
	shmem_ptr->count = 0;
	shmem_ptr->in = 0;
	shmem_ptr->out = 0;
	shmem_ptr->nextProducer = 0;
	shmem_ptr->bufferSize = bufferSize;

	// initialize the mutex lock
	pthread_mutexattr_t attrmutex;
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&shmem_ptr->lock, &attrmutex);

	// initialize the conditions 
	pthread_condattr_t attrcond;
	pthread_condattr_init(&attrcond);
	pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
	for(int i=0; i<3; i++) {
		pthread_cond_init(&shmem_ptr->producerCond[i], &attrcond);
	}

	for(int i=0; i<3; i++) {
		pthread_cond_init(&shmem_ptr->consumerCond[i], &attrcond);
	}

	// prepare for the child process initialization
	char keyString[10];
	sprintf(keyString, "%d", key);
	pid_t main_pid = getpid();

	for (int i = 0; i < 6; i++) {
		pid_t current_pid = getpid();
		if (current_pid == main_pid) {
			if (fork() == 0) {
				// child process
				if (i < 3) {
					// create producer processes
					printf("Creating producer process.\n");
					char colorCode[10];
					sprintf(colorCode, "%d", i);
					execl("./producer", "producer", keyString, colorCode, NULL);
				} else {
					// create consumer processes
					printf("Creating consumer process.\n");
					char colorCode[10];
					sprintf(colorCode, "%d", i - 3);
					execl("./consumer", "consumer", keyString, colorCode, NULL);
				}
			}
		}
	}

	int status;
	wait(&status); //wait all the process to terminate
	shmctl(shmem_id, IPC_RMID, NULL);
	printf("Program exiting.\n");
	return 0;
}