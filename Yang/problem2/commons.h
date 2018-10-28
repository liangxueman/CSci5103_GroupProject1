#include <pthread.h>
#include <sys/types.h>

const char* colors[3] = {"red", "green", "blue"};

#define buffer_size 6
#define number_deposits 1000

typedef struct item {
  int color_id;
  struct timeval timestamp;
} item;

typedef struct shared_content {
  item buffer[buffer_size];
  int count;
  int in;
  int out;
  pthread_cond_t spaceAvailable, itemAvailable;
  pthread_mutex_t wr_lock;
  /*
   * a cycle of color mutex locks to enforce the deposit and consuming order
   * only the first one would be unlocked at the initialization
   */
  pthread_mutex_t color_lock_producer[3];
  pthread_mutex_t color_lock_consumer[3];
} shared_content;
