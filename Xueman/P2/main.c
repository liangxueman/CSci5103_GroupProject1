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

	int bufferSize = atoi(argv[1]);
	int shmem_id;
	struct sharedContent* shmem_ptr;
	key_t key = 4456;  // a random key to access shared memory
	int size = sizeof(struct sharedContent) + sizeof(struct item) * bufferSize;
	// int size = 2048;
	int flag = 1023;  // all permissions and modes are set

	shmem_id = shmget(key, size, flag);
	if(shmem_id == -1) {
		printf("Fialed to shmget in main process.\n");
		exit(1);
	}

	shmem_ptr = shmat(shmem_id, (void*)NULL, flag);
	if(shmem_ptr == (void*)-1) {
		printf("Fialed to shmat in main process.\n");
		exit(2);
	}

	shmem_ptr->count = 0;
	shmem_ptr->in = 0;
	shmem_ptr->out = 0;
	shmem_ptr->nextProducer = 0;
	shmem_ptr->bufferSize = bufferSize;

	pthread_mutexattr_t attrmutex;
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&shmem_ptr->lock, &attrmutex);

	pthread_condattr_t attrcond;
	pthread_condattr_init(&attrcond);
	pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
	for(int i=0; i<3; i++) {
		pthread_cond_init(&shmem_ptr->producerCond[i], &attrcond);
	}

	for(int i=0; i<3; i++) {
		pthread_cond_init(&shmem_ptr->consumerCond[i], &attrcond);
	}

	char keyString[10];
  sprintf(keyString, "%d", key);
  pid_t main_pid = getpid();

  for (int i = 0; i < 6; i++) {
    pid_t current_pid = getpid();
    if (current_pid == main_pid) {
      if (fork() == 0) {
        if (i < 3) {
          printf("creating producer process \n");
          char colorCode[10];
          sprintf(colorCode, "%d", i);
          execl("./producer", "producer", keyString, colorCode, NULL);
        } else {
          printf("creating consumer process \n");
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