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
		printf("To execute the program, please use command: ./consumer (int)[key] (int)[colorCode]\n");
		return 0;
	}

	/*
	 * The colorCode is used to indicate colors
	 * red: 0
	 * green: 1
	 * blue: 2
	 */
	int colorCode = atoi(argv[2]);

	// shared memory identifier
	int shmIdentifier = shmget(atoi(argv[1]), 0, 0);
	if(shmIdentifier == -1) {
		printf("Failed to shmget in consumer process.\n");
		exit(1);
	}

	// shared memory pointer
	struct sharedContent* ptr = shmat(shmIdentifier, (void*)NULL, 1023); 
	if(ptr == (void*)-1) {
		printf("Fialed to shmat in consumer process.\n");
		exit(2);
	}

	// create the log files
	char filename[80] = "consumer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Failed to open log file in consumer process.\n");
		exit(1);
	}

	/*
	 * Shared buffer is a item pointer
	 * It points to the next position of the shared memory pointer
	 */
	struct item* buffer = (struct item*)(ptr + 1);

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&ptr->lock);

		while(ptr->count == 0 || buffer[ptr->out].colorCode != colorCode) {
			// if no avaialble item to consume or the current item is not the right color, then wait on condition
			while(pthread_cond_wait(&ptr->consumerCond[colorCode], &ptr->lock) != 0);
		}

		// to record the item consume timestamp
		struct timeval consumeTimestamp;
		gettimeofday(&consumeTimestamp, NULL);

		// write to log file
		fprintf(file,"%s %ld.%06u %ld.%06u\n", colors[colorCode], buffer[ptr->out].timestamp.tv_sec, buffer[ptr->out].timestamp.tv_usec, consumeTimestamp.tv_sec, consumeTimestamp.tv_usec);

		ptr->out = (ptr->out + 1) % ptr->bufferSize;
		ptr->count = ptr->count - 1;
		printf("One %s item has been removed\n", colors[colorCode]);

		// signal the next consumer to consume and the expected next producer to produce
		pthread_cond_signal(&ptr->consumerCond[((colorCode + 1) % 3)]);
		pthread_cond_signal(&ptr->producerCond[ptr->nextProducer]);
		pthread_mutex_unlock(&ptr->lock);
	}

	fclose(file);
	shmdt((void*)ptr);
	return 0;
}