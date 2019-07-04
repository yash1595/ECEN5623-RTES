#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS 2
#define THREAD_1 1
#define THREAD_2 2
#define CLOCK_SOURCE CLOCK_REALTIME
int time_val;

typedef struct
{
    int threadIdx;
} threadParams_t;
 
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

struct sched_param nrt_param;

pthread_mutex_t rsrcA, rsrcB;

volatile int rsrcACnt=0, rsrcBCnt=0, noWait=0;
void ReduceBackOff(pthread_t);

/*******************************************************************************
* @brief: Callback function for both threads. Mutexes are used to lock resources
*         Actual function is designed so as to cause deadlocks.
*******************************************************************************/
void *grabRsrcs(void *threadp)
{
   threadParams_t *threadParams = (threadParams_t *)threadp;
   int threadIdx = threadParams->threadIdx;


   if(threadIdx == THREAD_1)
   {
     printf("THREAD 1 grabbing resources\n");
     pthread_mutex_lock(&rsrcA);                                                // Acquires resource.
     rsrcACnt++;
     if(!noWait) usleep(1000000);
     printf("THREAD 1 got A, trying for B\n");
     if(rsrcACnt>0 && rsrcBCnt>0)                                               // Checks for deadlock.
     {
        printf("Thread-1:%x\n",threads[0]);
        ReduceBackOff(threads[0]);
     }
     rsrcBCnt++;
     printf("THREAD 1 got A and B\n");
     pthread_mutex_unlock(&rsrcB);                                              // Releases resource
     pthread_mutex_unlock(&rsrcA);
     printf("THREAD 1 done\n");
   }
   else
   {
     printf("THREAD 2 grabbing resources\n");
     pthread_mutex_lock(&rsrcB);
     rsrcBCnt++;
     if(!noWait) usleep(1000000);
     printf("THREAD 2 got B, trying for A\n");
     if(rsrcACnt>0 && rsrcBCnt>0)                                               // Checks for deadlock.
     {
        printf("Thread-2:%x\n",threads[1]);
        ReduceBackOff(threads[1]);
     }
     rsrcACnt++;
     printf("THREAD 2 got B and A\n");
     pthread_mutex_unlock(&rsrcA);
     pthread_mutex_unlock(&rsrcB);
     printf("THREAD 2 done\n");
   }
   pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
   int rc, safe=0;
   srand(time(NULL));  
   time_val = rand()%10000;
   printf("time_val:%d inv_time_val:%d\n",(time_val+1),(10000-time_val));
  
   rsrcACnt=0, rsrcBCnt=0, noWait=0;

   if(argc < 2)                                                                 // Takes arguments from user for safe or unsafe execution.
   {
     printf("Will set up unsafe deadlock scenario\n");
   }
   else if(argc == 2)
   {
     if(strncmp("safe", argv[1], 4) == 0)
       safe=1;
     else if(strncmp("race", argv[1], 4) == 0)
       noWait=1;
     else
       printf("Will set up unsafe deadlock scenario\n");
   }
   else
   {
     printf("Usage: deadlock [safe|race|unsafe]\n");
   }

   // Set default protocol for mutex
   pthread_mutex_init(&rsrcA, NULL);
   pthread_mutex_init(&rsrcB, NULL);

   printf("Creating thread %d\n", THREAD_1);                                    // Creates Thread-1
   threadParams[THREAD_1].threadIdx=THREAD_1;
   rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)&threadParams[THREAD_1]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 1 spawned\n");

   printf("Creating thread %d\n", THREAD_2);                                    // Creates Thread-2
   threadParams[THREAD_2].threadIdx=THREAD_2;
   rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)&threadParams[THREAD_2]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 2 spawned\n");

   printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
   printf("will try to join CS threads unless they deadlock\n");


   if(pthread_join(threads[0], NULL) == 0)                                      // Terminate threads.
     printf("Thread 1: %x done\n", (unsigned int)threads[0]);
   else
     perror("Thread 1");

   if(pthread_join(threads[1], NULL) == 0)
     printf("Thread 2: %x done\n", (unsigned int)threads[1]);
   else
     perror("Thread 2");

   if(pthread_mutex_destroy(&rsrcA) != 0)                                       // Destroy mutexes.
     perror("mutex A destroy");

   if(pthread_mutex_destroy(&rsrcB) != 0)
     perror("mutex B destroy");

   printf("All done\n");

   exit(0);
}

/*******************************************************************************
*@brief: This function removes the deadlock. It is invoked from thread[0] when 
         when a deadlock is detected. Resource-A obtained by thread[0] is 
         released so that thread[1] can complete execution. After which it is 
         locked again. Since, thread[1] has now finished execution, Mutex-B need 
         not be locked. 
*******************************************************************************/
void ReduceBackOff(pthread_t thread_id)
{
  if(thread_id == threads[0])
  {
      pthread_mutex_unlock(&rsrcA);
      usleep(time_val+1);
      pthread_mutex_lock(&rsrcA);
  }
  else
  {
      pthread_mutex_unlock(&rsrcB);
      usleep(10000-time_val);
      pthread_mutex_lock(&rsrcB);
  }

}