#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>

#define NUM_READERS 100
#define NUM_WRITERS 10

static int target = 0;
static sem_t rw_mutex;
static sem_t mutex;
int read_count = 0;

static void *writeThreadFunc(void *arg) {
  if (sem_wait(&rw_mutex) == -1) {
    printf("Error waiting for rw_mutex\n");
    exit(2);
  }

  //critical section
  target += 10;
  printf("Target: %d\n", target);
  //end critical section

  if (sem_post(&rw_mutex) == -1) {
    printf("Error signalling for rw_mutex\n");
    exit(2);
  }
}

static void *readThreadFunc(void *arg) {
  if (sem_wait(&mutex) == -1) {
    printf("Error waiting for mutex\n");
    exit(2);
  }
  read_count++;
  if (read_count == 1) {
    if (sem_wait(&rw_mutex) == -1) {
      printf("Error waiting for rw_mutex\n");
      exit(2);
    }
  }
  if (sem_post(&mutex) == -1) {
    printf("Error signalling for mutex\n");
    exit(2);
  }

  //critical section
  int local = target;
  printf("Got target: %d\n", local);
  printf("read_count: %d\n", read_count);
  //end critical section

  if (sem_wait(&mutex) == -1) {
    printf("Error waiting for mutex\n");
    exit(2);
  }
  read_count--;
  if (read_count == 0) {
    if (sem_post(&rw_mutex) == -1) {
      printf("Error signalling for rw_mutex\n");
      exit(2);
    }
  }

  if (sem_post(&mutex) == -1) {
    printf("Error signalling for mutex\n");
    exit(2);
  }
}

int main(int argc, char *argv[]) {

  pthread_t t1, t2, t3, t4;
  pthread_t readers[NUM_READERS];
  pthread_t writers[NUM_WRITERS];

  int status;

  if (argc != 2) {
    printf("Provide the number of loops as the only argument.\n");
    exit(1);
  }
  int loops;

  if (sscanf(argv[1], "%i", &loops) != 1) {
    printf("Argument not an integer.\n");
    exit(1);
  }

  //int loops = atoi(argv[1]);
  printf("Loops: %d\n", loops);

  if (sem_init(&rw_mutex, 0, 1) == -1) {
    printf("Error init rw_mutex\n");
    exit(1);
  }

  if (sem_init(&mutex, 0, 1) == -1) {
    printf("Error init mutex\n");
    exit(1);
  }

  int i;
  //Create NUM_READERS reader threads
  for (i = 0; i < NUM_READERS; i++) {
    status = pthread_create(&readers[i], NULL, readThreadFunc, &loops);
    if (status != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  //Create NUM_WRITES writer threads
  for (i = 0; i < NUM_WRITERS; i++) {
    status = pthread_create(&writers[i], NULL, writeThreadFunc, &loops);
    if (status != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  //Wait for reader threads
  for(i = 0; i < NUM_READERS; i++) {
    status = pthread_join(readers[i], NULL);
    if (status != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  //Wait for writer threads
  for(i = 0; i < NUM_WRITERS; i++) {
    status = pthread_join(writers[i], NULL);
    if (status != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  printf("target value %d \n", target);
  exit(0);
}
