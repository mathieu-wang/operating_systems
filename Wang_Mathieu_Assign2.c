#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>

#define NUM_READERS 100
#define NUM_WRITERS 10

static int target = 0;
static double maxWriteWait = 0;
static double maxReadWait = 0;
static double minWriteWait = 500000;
static double minReadWait = 500000;
static double totalReadWait = 0;
static double totalWriteWait = 0;
static sem_t rw_mutex;
static sem_t mutex;
int read_count = 0;

void sleep() {
  int sleepTime = rand() % 100; //generate random number between 0 and 100
  nanosleep((struct timespec[]){{0, sleepTime*1000000}}, NULL);
}

static void *writeThreadFunc(void *arg) {
  int i = 0;
  int numLoops = *(int*)arg;
  double totalThreadWaitTimeInMicros = 0;

  for(i = 0; i < numLoops; i++) {
    struct timeval tvBegin, tvEnd;
    gettimeofday(&tvBegin, NULL);

    if (sem_wait(&rw_mutex) == -1) {
      printf("Error waiting for rw_mutex\n");
      exit(2);
    }

    gettimeofday(&tvEnd, NULL);

    double iterationWaitTimeInMicros = (tvEnd.tv_sec - tvBegin.tv_sec)*1000000 + (tvEnd.tv_usec - tvBegin.tv_usec);
    totalThreadWaitTimeInMicros += iterationWaitTimeInMicros;

    //printf("Wait time: %f us\n", iterationWaitTimeInMicros);

    //critical section
    target += 10;
    printf("Target: %d\n", target);
    //end critical section

    if (sem_post(&rw_mutex) == -1) {
      printf("Error signalling for rw_mutex\n");
      exit(2);
    }
    sleep();
  }
  // printf("Total thread wait time: %f us\n", totalThreadWaitTimeInMicros);
  if (totalThreadWaitTimeInMicros < minWriteWait) {
    minWriteWait = totalThreadWaitTimeInMicros;
  } else if (totalThreadWaitTimeInMicros > maxWriteWait) {
    maxWriteWait = totalThreadWaitTimeInMicros;
  }
  totalWriteWait += totalThreadWaitTimeInMicros;
}

static void *readThreadFunc(void *arg) {
  int i = 0;
  int numLoops = *(int*)arg;
  double totalThreadWaitTimeInMicros = 0;

  for(i = 0; i < numLoops; i++) {
    struct timeval tvBegin, tvEnd;

    gettimeofday(&tvBegin, NULL);
    if (sem_wait(&mutex) == -1) {
      printf("Error waiting for mutex\n");
      exit(2);
    }
    gettimeofday(&tvEnd, NULL);

    //measure time waited for mutex
    double iterationWaitTimeInMicros = (tvEnd.tv_sec - tvBegin.tv_sec)*1000000 + (tvEnd.tv_usec - tvBegin.tv_usec);
    totalThreadWaitTimeInMicros += iterationWaitTimeInMicros;

    read_count++;
    if (read_count == 1) { //first reader thread, so need to get rw_mutex
      gettimeofday(&tvBegin, NULL);
      if (sem_wait(&rw_mutex) == -1) {
        printf("Error waiting for rw_mutex\n");
        exit(2);
      }
      gettimeofday(&tvEnd, NULL);
      //measure time waited for rw_mutex
      iterationWaitTimeInMicros = (tvEnd.tv_sec - tvBegin.tv_sec)*1000000 + (tvEnd.tv_usec - tvBegin.tv_usec);
      totalThreadWaitTimeInMicros += iterationWaitTimeInMicros;
    }
    if (sem_post(&mutex) == -1) {
      printf("Error signalling for mutex\n");
      exit(2);
    }

    //critical section
    int local = target;
    printf("Got target: %d\n", local);
    //printf("read_count: %d\n", read_count);
    //end critical section

    gettimeofday(&tvBegin, NULL);
    if (sem_wait(&mutex) == -1) {
      printf("Error waiting for mutex\n");
      exit(2);
    }
    gettimeofday(&tvEnd, NULL);
    //measure time waited for mutex
    iterationWaitTimeInMicros = (tvEnd.tv_sec - tvBegin.tv_sec)*1000000 + (tvEnd.tv_usec - tvBegin.tv_usec);
    totalThreadWaitTimeInMicros += iterationWaitTimeInMicros;

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
  if (totalThreadWaitTimeInMicros < minReadWait) {
    minReadWait = totalThreadWaitTimeInMicros;
  } else if (totalThreadWaitTimeInMicros > maxReadWait) {
    maxReadWait = totalThreadWaitTimeInMicros;
  }
  totalReadWait += totalThreadWaitTimeInMicros;
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

  printf("Loops: %d\n", loops);

  srand(time(NULL)); //initialized random

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

  printf("\nFinal target value %d \n", target);

  printf("Maximum Writer Wait Time: %f us\n", maxWriteWait);
  printf("Minimum Writer Wait Time: %f us\n", minWriteWait);
  printf("Average Writer Wait Time: %f us\n", totalWriteWait/NUM_WRITERS);
  printf("Maximum Reader Wait Time: %f us\n", maxReadWait);
  printf("Minimum Reader Wait Time: %f us\n", minReadWait);
  printf("Average Reader Wait Time: %f us\n", totalReadWait/NUM_READERS);

  exit(0);
}
