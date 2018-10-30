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
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char* argv[]) {
	if(argc != 3) {
		printf("To execute the program, please use command: ./producer (int)[key] (int)[colorCode]\n");
		return 0;
	}

	int colorCode = atoi(argv[2]);
	int shmIdentifier = shmget(atoi(argv[1]), 0, 0);
	if(shmIdentifier == -1) {
		printf("Failed to shmget in producer process.\n");
		exit(1);
	}

	struct sharedContent* ptr = shmat(shmIdentifier, (void*)NULL, 1023); 
	if(ptr == (void*)-1) {
		printf("Fialed to shmat in producer process.\n");
		exit(2);
	}

	char filename[80] = "producer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Failed to open log file in producer process.\n");
		exit(1);
	}
	
	struct item* buffer = (struct item*)(ptr + 1);

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&ptr->lock);
		while (ptr->count == ptr->bufferSize || ptr->nextProducer != colorCode) {
			while (pthread_cond_wait(&ptr->producerCond[colorCode], &ptr->lock) != 0);
		}

		// deposite one item to buffer
		buffer[ptr->in].colorCode = colorCode;
		gettimeofday(&buffer[ptr->in].timestamp, NULL);

		// write to log file
		fprintf(file, "%s %ld.%06u\n", colors[colorCode], buffer[ptr->in].timestamp.tv_sec, buffer[ptr->in].timestamp.tv_usec);

		ptr->count = ptr->count + 1;
		ptr->in = (ptr->in + 1) % ptr->bufferSize;
		ptr->nextProducer = (ptr->nextProducer + 1) % 3;
		printf("One %s item has been deposited\n", colors[colorCode]);

		// signal the corresponding consumer to consume and the next producer to produce
		pthread_cond_signal(&ptr->consumerCond[colorCode]);
		pthread_cond_signal(&ptr->producerCond[((colorCode + 1) % 3)]);
		pthread_mutex_unlock(&ptr->lock);
	}

	fclose(file);
	shmdt((void*)ptr);
	return 0;
}