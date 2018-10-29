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
		return (0);
	}

	int colorCode = atoi(argv[2]);
	int shmIdentifier = shmget(atoi(argv[1]), 0, 0);
	if(shmIdentifier == -1) {
		printf("Failed to shmget in consumer process.\n");
		exit(1);
	}

	struct sharedContent* ptr = shmat(shmIdentifier, (void*)NULL, 1023); 
	if((void*)ptr == -1) {
		printf("Fialed to shmat in consumer process.\n");
		exit(1);
	}

	char filename[80] = "consumer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Failed to open log file in consumer process.\n");
		exit(1);
	}

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&ptr[0].lock);

		while(ptr[0].count == 0 || ptr[0].buffer[ptr[0].out].colorCode != colorCode) {
			while(pthread_cond_wait(&ptr[0].consumerCond[colorCode], &ptr[0].lock) != 0);
		}

		fprintf(file,"%s %u.%06u\n", colors[colorCode], ptr[0].buffer[ptr[0].out].timestamp.tv_sec, ptr[0].buffer[ptr[0].out].timestamp.tv_usec);

		ptr[0].out = (ptr[0].out + 1) % bufferSize;
		ptr[0].count = ptr[0].count - 1;
		printf("One %s item has been removed\n", colors[colorCode]);

		pthread_cond_signal(&ptr[0].consumerCond[((colorCode + 1) % 3)]);
		pthread_cond_signal(&ptr[0].producerCond[ptr[0].nextProducer]);
		pthread_mutex_unlock(&ptr[0].lock);
	}

	fclose(file);
	shmdt((void*)ptr);
	return (0);
}