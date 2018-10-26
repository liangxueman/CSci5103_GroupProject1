#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

// TODO: (1) N should be the arugments (2) fix the code style (3) put common code and variables in commons.h

#define DEBUG

#define N 6
const char* colors[3];

typedef struct item {
  int color_id;
  struct timeval timestamp;
} item;

typedef struct shared_content {
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
//  if (argc != 2) {
//    printf("./prodcons N");
//  }

  colors[0] = "red";
  colors[1] = "green";
  colors[2] = "blue";

  int shmem_id;
  shared_content *shmem_ptr;

  key_t key = 4455;
  size_t size = 2048;
  int flag = 1023;

  // create a shared memory segment
  shmem_id = shmget(key, size, flag);
  if (shmem_id == -1) {
    printf("shmget failed\n");
    exit(1);
  }
  #ifdef DEBUG
  printf ("Got shmem id = %d \n",shmem_id);
  #endif

  // attach the new segment into my address space
  shmem_ptr = shmat(shmem_id, (void *) NULL, 1023);
  if (shmem_ptr == (void *)-1) {
    printf("shmat failed \n");
    exit(2);
  }
  #ifdef DEBUG
  printf("Got ptr = %p\n", shmem_ptr);
  #endif

  // initialize data in shared memory
  shmem_ptr[0].count = 0;
  shmem_ptr[0].in = 0;
  shmem_ptr[0].out = 0;
  // initialize condition variables
  pthread_condattr_t attrcond;
  pthread_condattr_init(&attrcond);
  pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&(shmem_ptr[0].spaceAvailable), &attrcond);
  pthread_cond_init(&(shmem_ptr[0].itemAvailable), &attrcond);
  /*
   * initialize mutex lock
   * one wr_lock, color locks for producers and consumers
   */
  pthread_mutexattr_t attrmutex;
  pthread_mutexattr_init(&attrmutex);
  pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&(shmem_ptr[0].wr_lock), &attrmutex);
  for (int i = 0; i < 3; i++) {
    pthread_mutex_init(&shmem_ptr[0].color_lock_producer[i], &attrmutex);
  }
  pthread_mutex_lock(&shmem_ptr[0].color_lock_producer[1]);
  pthread_mutex_lock(&shmem_ptr[0].color_lock_producer[2]);
  for (int i = 0; i < 3; i++) {
    pthread_mutex_init(&shmem_ptr[0].color_lock_consumer[i], &attrmutex);
  }
  pthread_mutex_lock(&shmem_ptr[0].color_lock_consumer[1]);
  pthread_mutex_lock(&shmem_ptr[0].color_lock_consumer[2]);
  printf("Finish initialization\n");

//   create producer and consumer process
  char keystr[10];
  sprintf(keystr, "%d", key);
  pid_t main_pid = getpid();

  for (int i = 0; i < 6; i++) {
    pid_t current_pid = getpid();
    if (current_pid == main_pid) {
      if (fork() == 0) {
        if (i < 3) {
          printf("creating producer process \n");
          char color_id[10];
          sprintf(color_id, "%d", i);
          execl("./producer", "producer", keystr, color_id, NULL);
        } else {
          printf("creating consumer process \n");
          char color_id[10];
          sprintf(color_id, "%d", i - 3);
          execl("./consumer", "consumer", keystr, color_id ,NULL);
        }
      }
    }
  }
  int status;
  wait(&status); //wait all the process to terminate
  shmctl(shmem_id, IPC_RMID, NULL);
}