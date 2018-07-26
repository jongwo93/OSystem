/* 
 * hw4 [<nconsumers>]
 * 
 * This problem has you solve the classic "bounded buffer" problem with
 * one producer and multiple consumers:
 *
 *                                       ------------
 *                                    /->| consumer |
 *                                    |  ------------
 *                                    |
 *  ------------                      |  ------------
 *  | producer |-->   bounded buffer --->| consumer |
 *  ------------                      |  ------------
 *                                    |
 *                                    |  ------------
 *                                    \->| consumer |
 *                                       ------------
 *
 *  The program below includes everything but the implementation of the
 *  bounded buffer itself.  main() does this:
 *
 *  1. starts N consumers as per the first argument (default 1)
 *  2. starts the producer
 *
 *  The producer reads positive integers from standard input and passes those
 *  into the buffer.  The consumers read those integers and "perform a
 *  command" based on them (all they really do is sleep for some period...)
 *
 *  on EOF of stdin, the producer passes N copies of -1 into the buffer.
 *  The consumers interpret -1 as a signal to exit.
 */

#define _REENTRANT
#include <stdio.h>
#include <stdlib.h>             /* atoi() */
#include <unistd.h>             /* usleep() */
#include <assert.h>             /* assert() */
#include <signal.h>             /* signal() */
#include <alloca.h>             /* alloca() */
#include <stdint.h>
#include <pthread.h>

/**************************************************************************\
 *                                                                        *
 * Bounded buffer.  This is the only part you need to modify.  Your       *
 * buffer should have space for up to 10 integers in it at a time.        *
 *                                                                        *
 * Add any data structures you need (globals are fine) and fill in        *
 * implementations for these three procedures:                            *
 *                                                                        *
 * void buffer_init(void)                                                 *
 *                                                                        *
 *      buffer_init() is called by main() at the beginning of time to     *
 *      perform any required initialization.  I.e. initialize the buffer, *
 *      any mutex/condition variables, etc.                               *
 *                                                                        *
 * void buffer_insert(int number)                                         *
 *                                                                        *
 *      buffer_insert() inserts a number into the next available slot in  *
 *      the buffer.  If no slots are available, the thread should wait    *
 *      (not spin-wait!) for an empty slot to become available.           *
 *                                                                        *
 * int buffer_extract(void)                                               *
 *                                                                        *
 *      buffer_extract() removes and returns the number in the next       *
 *      available slot.  If no number is available, the thread should     *
 *      wait (not spin-wait!) for a number to become available.  Note     *
 *      that multiple consumers may call buffer_extract() simulaneously.  *
 *                                                                        *
\**************************************************************************/

pthread_mutex_t mutex;
pthread_cond_t slot_avail;
pthread_cond_t num_avail;
int size;

struct buffer_node {
  int data;
  struct buffer_node* next;
};

struct buffer_node* head = NULL;
struct buffer_node* tail = NULL;

void push(int number) {
  
  //pthread_mutex_lock(&mutex);
  struct buffer_node* temp = (struct buffer_node*)malloc(sizeof(struct buffer_node));
  temp -> data = number;
  temp -> next = NULL;
  if(head == NULL && tail == NULL) {
    head = temp;
    tail = temp;
  } else {
    tail -> next = temp;
    tail = temp;
  }
  size++;
  //pthread_mutex_unlock(&mutex);

}

int pop() {
  
  if(head == NULL) {
    return 0;
  }

  struct buffer_node* temp = head;
  int retVal;

  if(head == tail) {
    head = NULL;
    tail = NULL;
  } else {
    head = head -> next;
  }
  retVal = temp -> data;
  free(temp);
  size--;
  //pthread_mutex_unlock(&mutex);
  return retVal;
}

void buffer_init(void)
{
  //printf("initiating \n");
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&slot_avail, NULL);
  pthread_cond_init(&num_avail, NULL);
  size = 0;

  //printf("init finished \n");
  //printf("buffer_init called: doing nothing\n"); /* FIX ME */
}

void buffer_insert(int number)
{
  //printf("inserting \n");
  pthread_mutex_lock(&mutex);
  if(size == 10) {
    //printf("Full, no slot avail, waiting...\n");
    pthread_cond_wait(&slot_avail, &mutex);
  }
  //printf("Not full, slot is available\n");
  push(number);
  pthread_cond_broadcast(&num_avail);
  pthread_mutex_unlock(&mutex);
  //printf("insert finished \n");

  //printf("buffer_insert called with %d: doing nothing\n", number); /* FIX ME */
}

int buffer_extract(void)
{

  pthread_mutex_lock(&mutex);
  //printf("extracting \n");]
  if(size == 0) {
    pthread_cond_wait(&num_avail, &mutex);
  }
  int retVal;
  retVal = pop();
  pthread_cond_broadcast(&slot_avail);
  pthread_mutex_unlock(&mutex);
  
  //printf("extracting finished , VALUE : %d \n", retVal);
  //printf("buffer_extract called: returning -1\n"); /* FIX ME */
  return retVal;                   
  /* FIX ME */
}

/**************************************************************************\
 *                                                                        *
 * consumer thread.  main starts any number of consumer threads.  Each    *
 * consumer thread reads and "interprets" numbers from the bounded        *
 * buffer.                                                                *
 *                                                                        *
 * The interpretation is as follows:                                      *
 *                                                                        *
 * o  positive integer N: sleep for N * 100ms                             *
 * o  negative integer:  exit                                             *
 *                                                                        *
 * Note that a thread procedure (called by pthread_create) is required to *
 * take a void * as an argument and return a void * as a result.  We play *
 * a dirty trick: we pass the thread number (main()'s idea of the thread  *
 * number) as the "void *" argument.  Hence the casts.  The return value  *
 * is ignored so we return NULL.                                          *
 *                                                                        *
\**************************************************************************/

void *consumer_thread(void *raw_consumerno)
{
  int consumerno = (int)(intptr_t)raw_consumerno; /* (slightly) dirty trick to pass in an integer */

  printf("  consumer %d: starting\n", consumerno);
  while (1)
    {
      int number = buffer_extract();

      if (number < 0)
        break;

      usleep(100000 * number);  /* "interpret" the command */
      printf("  consumer %d: %d\n", consumerno, number);
      fflush(stdout);
    }

  printf("  consumer %d: exiting\n", consumerno);
  return(NULL);
}

/**************************************************************************\
 *                                                                        *
 * producer.  main calls the producer as an ordinary procedure rather     *
 * than creating a new thread.  In other words the original "main" thread *
 * becomes the "producer" thread.                                         *
 *                                                                        *
 * The producer reads numbers from stdin, and inserts them into the       *
 * bounded buffer.  On EOF from stdin, it finished up by inserting a -1   *
 * for every consumer thread so that all the consumer thread shut down    *
 * cleanly.                                                               *
 *                                                                        *
\**************************************************************************/

#define MAXLINELEN 128

void producer(int nconsumers)
{
  char buffer[MAXLINELEN];
  int number;

  printf("  producer: starting\n");

  while (fgets(buffer, MAXLINELEN, stdin) != NULL)
    {
      number = atoi(buffer);
      printf("producer: %d\n", number);
      buffer_insert(number);
    }

  printf("producer: read EOF, sending %d '-1' numbers\n", nconsumers);
  for (number = 0; number < nconsumers; number++)
    buffer_insert(-1);

  printf("producer: exiting\n");
}

/*************************************************************************\
 *                                                                       *
 * main program.  Main calls buffer_init and does other initialization,  *
 * fires of N copies of the consumer (N given by command-line argument), *
 * then calls the producer.                                              *
 *                                                                       *
\*************************************************************************/

int main(int argc, char *argv[])
{
  pthread_t *consumers;
  int nconsumers = 1;
  int kount;

  if (argc > 1)
    nconsumers = atoi(argv[1]);

  printf("main: nconsumers = %d\n", nconsumers);
  assert(nconsumers >= 0);

  /*
   * 1. initialization
   */
  buffer_init();
  signal(SIGALRM, SIG_IGN);     /* evil magic for usleep() under solaris */

  /*
   * 2. start up N consumer threads
   */
  consumers = (pthread_t *)alloca(nconsumers * sizeof(pthread_t));
  for (kount = 0; kount < nconsumers; kount++)
    {
      int test = pthread_create(&consumers[kount], /* pthread number */
                                NULL,            /* "attributes" (unused) */
                                consumer_thread, /* procedure */
                                (void *)(intptr_t)kount);  /* (slightly) dirty trick to pass consumer number */

      assert(test == 0);
    }

  /*
   * 3. run the producer in this thread.
   */
  producer(nconsumers);

  /*
   * n. clean up: the producer told all the consumers to shut down (by
   *    sending -1 to each).  Now wait for them all to finish.
   */
  for (kount = 0; kount < nconsumers; kount++)
    {
      int test = pthread_join(consumers[kount], NULL);

      assert(test == 0);
    }
  return(0);
}
