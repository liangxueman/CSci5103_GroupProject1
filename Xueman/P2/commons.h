#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>

#define bufferSize 6
#define iteration 20

const char* colors[3] = {"red", "green", "blue"};

struct item {
  int colorCode;
  struct timeval timestamp;
};

struct sharedContent {
  struct item buffer[bufferSize];
  int count;
  int in;
  int out;
  int nextProducer;
  pthread_mutex_t lock;
  pthread_cond_t producerCond[3];
  pthread_cond_t consumerCond[3];
};