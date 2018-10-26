#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define DEBUG

#define N 6
const char* colors[3];
int number_deposits = 1000;

typedef struct item {
  int color_id;
  struct timeval timestamp;
} item;

typedef struct shared_content{
  item buffer[N];
  int count;
  int in;
  int out;
  pthread_cond_t spaceAvailable, itemAvailable;
  pthread_mutex_t wr_lock;
  pthread_mutex_t color_lock_producer[3];
  pthread_mutex_t color_lock_consumer[3];
} shared_content;


int main(int argc, char *argv[]) {
  //  if (argc != 3) {
//    printf("./prodcons N");
//  }

  colors[0] = "red";
  colors[1] = "green";
  colors[2] = "blue";
  int color_id = atoi(argv[2]);

  int shared_id = shmget(atoi(argv[1]), 0, 0);
  if (shared_id == -1) {
    printf("child shmget failed\n");
    exit(1);
  }

  // attach the segment into the address space
  shared_content* ptr = shmat (shared_id, (void *) NULL, 1023);
  if (ptr == (void *) -1) {
    printf("child shmat failed\n");
    exit(2);
  }

//   creat output log file
  char file_name[100] = "consumer_";
  strcat(file_name, colors[color_id]);
  char file_extend[10] = ".log";
  strcat(file_name, file_extend);
  FILE *f = fopen(file_name, "w");
  if (f == NULL){
    printf("Error opening file!\n");
    return 0;
  }

  for (int i = 0; i < number_deposits; i++) {
    // wait on its own color lock to enforce the  order
    pthread_mutex_lock(&ptr[0].color_lock_consumer[color_id]);
    pthread_mutex_lock(&ptr[0].wr_lock);
    // enter critical section
    while (ptr[0].count == 0)
      // consumer blocked when buffer is empty
      while (pthread_cond_wait(&ptr[0].itemAvailable, &ptr[0].wr_lock) != 0);

    // consume
    fprintf(f,"%s %u.%06u\n", colors[ptr[0].buffer[ptr[0].out].color_id], ptr[0].buffer[ptr[0].out].timestamp.tv_sec,
            ptr[0].buffer[ptr[0].out].timestamp.tv_usec);
    printf("consumer_%s consume %d/%d, item color %s \n", colors[color_id],
           i, number_deposits, colors[ptr[0].buffer[ptr[0].out].color_id]);

    ptr[0].count = ptr[0].count - 1;
    ptr[0].out = (ptr[0].out + 1) % N;
    //exit critical section
    pthread_mutex_unlock(&ptr[0].wr_lock);
    pthread_cond_signal(&ptr[0].spaceAvailable);
    // free the next color consumer
    pthread_mutex_unlock(&ptr[0].color_lock_consumer[(color_id + 1) % 3]);
  }

  fclose(f);
  shmdt ( (void *) ptr);
}