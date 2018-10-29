#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

struct item {
	int colorCode;
	struct timeval timestamp;
};

#define iteration 1000

int bufferSize; 
struct item* buffer;
const char* colors[3] = {"red", "green", "blue"};

int count = 0;
int in = 0;
int out = 0;
int nextProducer = 0;

pthread_mutex_t lock;
pthread_cond_t producerCond[3];
pthread_cond_t consumerCond[3];

void* producer(int colorCode) {
	char filename[80] = "producer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Fialed to open log file.\n");
		exit(1);
	}

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&lock);
		while (count == bufferSize || nextProducer != colorCode) {
			while (pthread_cond_wait(&producerCond[colorCode], &lock) != 0);
		}

		buffer[in].colorCode = colorCode;
		gettimeofday(&buffer[in].timestamp, NULL);
		fprintf(file, "%s %u.%06u\n", colors[colorCode], buffer[in].timestamp.tv_sec, buffer[in].timestamp.tv_usec);

		count = count + 1;
		in = (in + 1) % bufferSize;
		printf("One %s item has been deposited\n", colors[colorCode]);
		nextProducer = (nextProducer + 1) % 3;

		pthread_cond_signal(&consumerCond[colorCode]);
		pthread_cond_signal(&producerCond[((colorCode + 1) % 3)]);
		pthread_mutex_unlock(&lock);
	}
	fclose(file);
}

void* consumer(int colorCode) {
	char filename[80] = "consumer_";
	strcat(filename, colors[colorCode]);
	strcat(filename, ".log");
	FILE *file = fopen(filename, "w");
	if(file == NULL) {
		printf("Failed to open log file\n");
		exit(1);
	}

	for(int i=0; i<iteration; i++) {
		pthread_mutex_lock(&lock);

		while(count == 0 || buffer[out].colorCode != colorCode) {
			while(pthread_cond_wait(&consumerCond[colorCode], &lock) != 0);
		}

		fprintf(file,"%s %u.%06u\n", colors[colorCode], buffer[out].timestamp.tv_sec, buffer[out].timestamp.tv_usec);

		out = (out + 1) % bufferSize;
		count = count - 1;
		printf("One %s item has been removed\n", colors[colorCode]);

		pthread_cond_signal(&consumerCond[((colorCode + 1) % 3)]);
		pthread_cond_signal(&producerCond[nextProducer]);
		pthread_mutex_unlock(&lock);
	}
	fclose(file);
}


int main(int argc, char const *argv[]) {
	if(argc != 2) {
		printf("To execute the program, please use command: ./prodcons [(int)bufferSize].\n");
		return(0);
	}
	bufferSize = atoi(argv[1]);
	if(bufferSize <=0 ) {
		printf("Please enter a POSITIVE integer to initialize the buffer.\n");
		return(0);
	}

	buffer= malloc(sizeof(struct item) * bufferSize);

	pthread_t producerThreads[3];
	pthread_t consumerThreads[3];
	pthread_mutex_init(&lock, NULL);

	for(int i=0; i<3; i++) {
		pthread_cond_init(&consumerMutex[i], NULL);
		pthread_cond_init(&producerMutex[i], NULL);
	}

	int n;
	for(int i=0; i<3; i++) {
		if(n = pthread_create(&producerThreads[i], NULL, producer, i)) {
			printf("Failed to create %s producer.\n", colors[i]);
			exit(1);
		}
	}

	for(int i=0; i<3; i++) {
		if(n = pthread_create(&consumerThreads[i], NULL, consumer, i)) {
			printf("Failed to create %s consumer.\n", colors[i]);
			exit(1);
		}
	}

	for(int i=0; i<3; i++) {
		if(n = pthread_join(consumerThreads[i], NULL)) {
			printf("Error in pthread_join\n");
			exit(1);
		}
		// if(n = pthread_join(producerThreads[i], NULL)) {
		// 	printf("Error in pthread join\n");
		// 	exit(1);
		// }
	}

	free(buffer);
	printf("Program exiting.\n");
	return 0;
}