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

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("./producer key color_id\n");
  }

  // color identifier
  int color_id = atoi(argv[2]);

  // shared memory identifier
  int shared_id = shmget(atoi(argv[1]), 0, 0);
  if (shared_id == -1) {
    printf("child shmget failed\n");
    exit(1);
  }

  // attach the segment into the address space
  shared_content* ptr = shmat (shared_id, (void*) NULL, 1023);
  if (ptr == (void*) -1) {
    printf("child shmat failed\n");
    exit(2);
  }

  // creat output log file
  char file_name[100] = "producer_";
  strcat(file_name, colors[color_id]);
  char file_extend[10] = ".log";
  strcat(file_name, file_extend);
  FILE* f = fopen(file_name, "w");
  if (f == NULL){
    printf("Error opening file!\n");
    return 0;
  }

  struct item* buffer = (struct item*)(ptr + 1);

  for (int i = 0; i < number_deposits; i++) {
    // wait on its own color lock to enforce the deposit order
    pthread_mutex_lock(&ptr[0].color_lock_producer[color_id]);
    pthread_mutex_lock(&ptr[0].wr_lock);
    // enter critical section
    while (ptr[0].count == ptr[0].buffer_size)
      // producer blocked when buffer is full
      while (pthread_cond_wait(&ptr[0].spaceAvailable, &ptr[0].wr_lock) != 0);

    // deposit
    buffer[ptr[0].in].color_id = color_id;
    gettimeofday(&buffer[ptr[0].in].timestamp, NULL);
    fprintf(f, "%s %lu.%06lu\n", colors[color_id],
        buffer[ptr[0].in].timestamp.tv_sec, buffer[ptr[0].in].timestamp.tv_usec);
    printf("producer_%s deposit %d/%d \n", colors[color_id], i + 1, number_deposits);

    ptr[0].count = ptr[0].count + 1;
    ptr[0].in = (ptr[0].in + 1) % ptr[0].buffer_size;

    // exit critical section
    pthread_mutex_unlock(&ptr[0].wr_lock);
    pthread_cond_signal(&ptr[0].itemAvailable);
    // free the next color producer
    pthread_mutex_unlock(&ptr[0].color_lock_producer[(color_id + 1) % 3]);
  }
  fclose(f);
  shmdt ((void*) ptr);
}