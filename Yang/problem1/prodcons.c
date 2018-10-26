#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#define number_deposits 1000

typedef struct item {
  int color_id;
  struct timeval timestamp;
} item;

int buffer_size;
struct item* buffer;
int count = 0;
int in = 0;
int out = 0;

const char* colors[3];

pthread_cond_t spaceAvailable, itemAvailable;
pthread_mutex_t wr_lock;
pthread_mutex_t color_lock_producer[3];
pthread_mutex_t color_lock_consumer[3];

void* producer(int color_id) {
  char file_name[100] = "producer_";
  strcat(file_name, colors[color_id]);
  char file_extend[10] = ".log";
  strcat(file_name, file_extend);
  FILE *f = fopen(file_name, "w");
  if (f == NULL){
    printf("Error opening file!\n");
    return 0;
  }
  for (int i = 0; i < number_deposits; i++) {
    // wait on its own color lock to enforce the deposit order
    pthread_mutex_lock(&color_lock_producer[color_id]);
    pthread_mutex_lock(&wr_lock);
    // enter critical section
    while (count == buffer_size)
      // producer blocked when buffer is full
      while (pthread_cond_wait(&spaceAvailable, &wr_lock) != 0);

    // deposit
    buffer[in].color_id = color_id;
    gettimeofday(&buffer[in].timestamp, NULL);
    fprintf(f, "%s %u.%06u\n", colors[color_id], buffer[in].timestamp.tv_sec,
            buffer[in].timestamp.tv_usec);
    printf("producer_%s deposit %d/%d \n", colors[color_id], i, number_deposits);

    count = count + 1;
    in = (in+1) % buffer_size;

    // exit critical section
    pthread_mutex_unlock(&wr_lock);
    pthread_cond_signal(&itemAvailable);
    // free the next color producer
    pthread_mutex_unlock(&color_lock_producer[(color_id + 1) % 3]);
  }
  fclose(f);
}

void* consumer(int color_id) {
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
    pthread_mutex_lock(&color_lock_consumer[color_id]);
    pthread_mutex_lock(&wr_lock);
    // enter critical section
    while (count == 0)
      // consumer blocked when buffer is empty
      while (pthread_cond_wait(&itemAvailable, &wr_lock) != 0);

    // consume
    fprintf(f,"%s %u.%06u\n", colors[buffer[out].color_id], buffer[out].timestamp.tv_sec,
            buffer[out].timestamp.tv_usec);
    printf("consumer_%s consume %d/%d, item color %s \n", colors[color_id],
           i, number_deposits, colors[buffer[out].color_id]);

    count = count - 1;
    out = (out + 1) % buffer_size;
    //exit critical section
    pthread_mutex_unlock(&wr_lock);
    pthread_cond_signal(&spaceAvailable);
    // free the next color consumer
    pthread_mutex_unlock(&color_lock_consumer[(color_id + 1) % 3]);
  }
  fclose(f);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("./prodcons buffer_size \n");
    exit(0);
  }

  buffer_size = atoi(argv[1]);
  assert(buffer_size > 0);
  buffer= malloc(sizeof(struct item) * buffer_size);

  colors[0] = "red";
  colors[1] = "green";
  colors[2] = "blue";

  pthread_t producer_tid[3];
  pthread_t consumer_tid[3];
  pthread_mutex_init(&wr_lock, NULL);
  for (int i = 0; i < 3; i++) {
    pthread_mutex_init(&color_lock_producer[i], NULL);
  }
  pthread_mutex_lock(&color_lock_producer[1]);
  pthread_mutex_lock(&color_lock_producer[2]);
  for (int i = 0; i < 3; i++) {
    pthread_mutex_init(&color_lock_consumer[i], NULL);
  }
  pthread_mutex_lock(&color_lock_consumer[1]);
  pthread_mutex_lock(&color_lock_consumer[2]);
  pthread_cond_init(&spaceAvailable, NULL);
  pthread_cond_init(&itemAvailable, NULL);

  for (int i = 0; i < 3; i++) {
    pthread_create(&producer_tid[i], NULL, producer, i);
    pthread_create(&consumer_tid[i], NULL, consumer, i);
  }
  for (int j = 0; j < 3; j++) {
    pthread_join(producer_tid[j], NULL);
    pthread_join(consumer_tid[j], NULL);
  }

  free(buffer);
  printf("program finished\n");
  return 0;
}
