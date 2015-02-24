#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

#define NUM_READERS 100
#define NUM_WRITERS 10

static int target = 0;
static sem_t sem;


static void *threadFunc(void *arg) {

  int loops = *((int *) arg);
  int loc, j;

  for (j = 0; j < loops; j++) {

     if (sem_wait(&sem) == -1)
       exit(2);

    loc = target;
    loc++;
    target = loc;

        if (sem_post(&sem) == -1)
         exit(2);
  }
  return NULL;
}



int main(int argc, char *argv[]) {

  pthread_t t1, t2, t3, t4;
  pthread_t readers[NUM_READERS];
  pthread_t writers[NUM_WRITERS];

  int status;

  int loops = 100000;

  if (sem_init(&sem, 0, 1) == -1) {
    printf("Error, init semaphore\n");
    exit(1);
  }

  int i;
  //Create NUM_READERS readers threads
  for (i = 0; i < NUM_READERS; i++) {
    status = pthread_create(&readers[i], NULL, threadFunc, &loops);
    if (status != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  for(i = 0; i < NUM_READERS; i++) {
    status = pthread_join(readers[i], NULL);
    if (status != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  printf("target value %d \n", target);
  exit(0);
}
