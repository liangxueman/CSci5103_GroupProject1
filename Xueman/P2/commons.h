/* 
  * CSci5103 Fall 2018
  * Assignment 4
  * Group Memebers: Xueman Liang, Yang Yang
  * Student IDs: 4271136, 5305584
  * x500: liang195, yang5276
  * CSELABS Machine: UMN VOLE CSE-IT (http://vole.cse.umn.edu/) Virtual Linux Machine
*/

#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>

#define iteration 1000

// producer and consuer color id array
const char* colors[3] = {"red", "green", "blue"};

struct item {
  int colorCode;
  struct timeval timestamp;
};

struct sharedContent {
  int bufferSize;
  int count;
  int in;
  int out;
  int nextProducer;
  pthread_mutex_t lock;
  pthread_cond_t producerCond[3];
  pthread_cond_t consumerCond[3];
};