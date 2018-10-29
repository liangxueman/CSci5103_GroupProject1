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

	int colorCode = atoi(argv[2]);
	int shmIdentifier = shmget(atoi(argv[1]), 0, 0);
	if(shmIdentifier == -1) {
		printf("Failed to shmget in consumer process.\n");
		exit(1);
	}

	struct sharedContent* ptr = shmat(shmIdentifier, (void*)NULL, 1023); 
	if(ptr == (void*)-1) {
		printf("Fialed to shmat in consumer process.\n");
		exit(2);
	}

	char filename[80] = "consumer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Failed to open log file in consumer process.\n");
		exit(1);
	}

	struct item* buffer = (struct item*)(ptr + 1);

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&ptr->lock);

		while(ptr->count == 0 || buffer[ptr->out].colorCode != colorCode) {
			while(pthread_cond_wait(&ptr->consumerCond[colorCode], &ptr->lock) != 0);
		}

		fprintf(file,"%s %u.%06u\n", colors[colorCode], buffer[ptr->out].timestamp.tv_sec, buffer[ptr->out].timestamp.tv_usec);

		ptr->out = (ptr->out + 1) % ptr->bufferSize;
		ptr->count = ptr->count - 1;
		printf("One %s item has been removed\n", colors[colorCode]);

		pthread_cond_signal(&ptr->consumerCond[((colorCode + 1) % 3)]);
		pthread_cond_signal(&ptr->producerCond[ptr->nextProducer]);
		pthread_mutex_unlock(&ptr->lock);
	}

	fclose(file);
	shmdt((void*)ptr);
	return 0;
}