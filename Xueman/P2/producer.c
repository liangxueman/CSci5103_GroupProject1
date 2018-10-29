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
		return (0);
	}

	int colorCode = atoi(argv[2]);
	int shmIdentifier = shmget(atoi(argv[1]), 0, 0);
	if(shmIdentifier == -1) {
		printf("Failed to shmget in producer process.\n");
		exit(1);
	}

	struct sharedContent* ptr = shmat(shmIdentifier, (void*)NULL, 1023); 
	if((void*)ptr == -1) {
		printf("Fialed to shmat in producer process.\n");
		exit(1);
	}

	char filename[80] = "producer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Failed to open log file in producer process.\n");
		exit(1);
	}

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&ptr[0].lock);
		while (ptr[0].count == bufferSize || ptr[0].nextProducer != colorCode) {
			while (pthread_cond_wait(&ptr[0].producerCond[colorCode], &ptr[0].lock) != 0);
		}

		ptr[0].buffer[ptr[0].in].colorCode = colorCode;
		gettimeofday(&ptr[0].buffer[ptr[0].in].timestamp, NULL);
		fprintf(file, "%s %u.%06u\n", colors[colorCode], ptr[0].buffer[ptr[0].in].timestamp.tv_sec, ptr[0].buffer[ptr[0].in].timestamp.tv_usec);

		ptr[0].count = ptr[0].count + 1;
		ptr[0].in = (ptr[0].in + 1) % bufferSize;
		printf("One %s item has been deposited\n", colors[colorCode]);
		ptr[0].nextProducer = (ptr[0].nextProducer + 1) % 3;

		pthread_cond_signal(&ptr[0].consumerCond[colorCode]);
		pthread_cond_signal(&ptr[0].producerCond[((colorCode + 1) % 3)]);
		pthread_mutex_unlock(&ptr[0].lock);
	}

	fclose(file);
	shmdt((void*)ptr);
	return (0);
}