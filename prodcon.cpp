#include "sharedMemory.h"
#include <pthread.h>
#include <semaphore.h> //define the sem_t type, used in performing semaphore operations
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

using namespace std;

extern unsigned int ip_checksum(unsigned char *data, int length);

int nitems;
int buffer_size;
// his operation returns with the mutex object referenced by mutex in the locked state with the calling thread as its owner
pthread_mutex_t mutex;
// define the sem_t type objects, used in performing semaphore operations
sem_t *full = NULL;
sem_t *empty = NULL;
sem_t *semid = NULL;

void signal_handler(int sig)
{
  printf("\nExiting\n");
  exit(1);
}

void producer()
{
  int in = 0;
  item next_produced;
  next_produced.item_no = 0;

  while (true)
  {
    sem_wait(empty);
    pthread_mutex_lock(&mutex);

    /* produce an item in next_produced */
    /* 1. Increment the buffer count (item_no)  */
    next_produced.item_no++;

    for (int i = 0; i < PAYLOAD_SIZE; i++)
    {
      /* 3. Generate the payload data             */
      next_produced.payload[i] = (unsigned char)rand() % 256;
    }

    /* 2. Calculate the 16-bit checksum (cksum) */
    next_produced.cksum = (unsigned short)ip_checksum(&next_produced.payload[0], PAYLOAD_SIZE);

    memcpy((void *)&buffer_item[in], (void *)&next_produced, sizeof(item));

    in = (in + 1) % nitems;
    pthread_mutex_unlock(&mutex);
    sem_post(full);
  }

  pthread_exit(0);
}

void consumer()
{
  int out = 0;
  item next_consumed;
  unsigned short cksum1, cksum2;

  while(true)
  {
    sem_wait(full);
    pthread_mutex_lock(&mutex);
    memcpy((void *)&next_consumed, (void *)&buffer_item[out], sizeof(item));
    out = (out + 1) % nitems;
    cksum2 = next_consumed.cksum;
    cksum1 = (unsigned short)ip_checksum(&next_consumed.payload[0], PAYLOAD_SIZE);

    /* 2. Verify the calculated checksum matches what is stored in next_consumed */
    if (cksum1 != cksum2)
    {
      printf("Checksum mismatch: received 0x%x, expected 0x%x \n", cksum2, cksum1);
    }
    sem_post(empty);
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(0);
}

// Got help from Gabriela Pinto
int main(int argc, char *argv[])
{
  pthread_t prod, cons;
  pthread_attr_t attr; /* set of thread attributes */

  // change the action taken by a process on receipt of a specific signal
  struct sigaction act;
  // specifies the action to be associated with signum and may be SIG_DFL for the default action
  act.sa_handler = signal_handler;
  //initializes the signal set given by set to empty, with all signals excluded from the set
  sigemptyset(&act.sa_mask);
  //specifies a set of flags which modify the behavior of the signal
  act.sa_flags = 0;
  sigaction(SIGINT, &act, 0);

  // check if the user input has at least 2 arguments
  if (argc < 2)
  {
    printf("Usage: %s <nbytes> \n", argv[0]);
    return -1;
  }
  nitems = atoi(argv[1]);
  // Check nitems size
  if (nitems * 40 > MEMSIZE || nitems <= 0)
  {
    printf("Too many items\n");
    return -1;
  }

  //opening all 3 semaphors
  //initialize and open a named semaphore
  full = sem_open("full", O_CREAT | O_EXCL, 0644, 0);
  if (full == NULL)
  {
    perror("Semaphore initialization failed");
    return -1;
  }
  empty = sem_open("empty", O_CREAT | O_EXCL, 0644, 0);
  if (empty == NULL)
  {
    perror("Semaphore initialization failed");
    return -1;
  }
  semid = sem_open("semid", O_CREAT | O_EXCL, 0644, 0);
  if (semid == NULL)
  {
    perror("Semaphore initialization failed");
    return -1;
  }

  //opening Mutex
  pthread_mutex_init(&mutex, NULL);

  //getting default attributes
  pthread_attr_init(&attr);

  //producer funct, create a new thread
  //thread_create(&tid,&attr,thread_function, NULL);
  pthread_create(&prod, &attr, (void *_Nullable (*)(void *))producer, NULL);
  //consumer funct
  pthread_create(&cons, &attr, (void *_Nullable (*)(void *))consumer, NULL);

  /* wait for the thread to exit */
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);

  sem_unlink("semid");
  pthread_mutex_destroy(&mutex);
  printf("finish\n");
  return 0;
}