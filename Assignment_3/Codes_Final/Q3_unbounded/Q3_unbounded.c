#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS		      4                                                   

                                                                                // Assignment of priority to services
#define START_SERVICE 		  0
#define HIGH_PRIO_SERVICE 	1
#define MID_PRIO_SERVICE 	  2
#define LOW_PRIO_SERVICE 	  3
#define NUM_MSGS 		   3

pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param nrt_param;

typedef struct
{
    int threadIdx;
} threadParams_t;


threadParams_t threadParams[NUM_THREADS];

pthread_mutex_t msgSem;
pthread_mutexattr_t rt_safe;

int rt_protocol;

volatile int runInterference=0, CScount=0;
volatile unsigned long long idleCount[NUM_THREADS];
int intfTime=0;


void *startService(void *threadid);

unsigned const int seqIterations = 47;
unsigned const int Iterations = 1000;

                                                                                // Macro to calculate Fibonacci series
#define FIB_TEST(seqCnt, iterCnt)      \
   for(idx=0; idx < iterCnt; idx++)    \
   {                                   \
      fib = fib0 + fib1;               \
      while(jdx < seqCnt)              \
      {                                \
         fib0 = fib1;                  \
         fib1 = fib;                   \
         fib = fib0 + fib1;            \
         jdx++;                        \
      }                                \
      jdx=0;                           \
   }                                   \

/*******************************************************************************
 @brief: This call back function belongs to Med priority task. It does not 
 *         requrire semaphores.
*******************************************************************************/
void *idleNoSem(void *threadp)
{
  struct timespec timeNow;
  unsigned int idx = 0, jdx = 1;
  volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;
  threadParams_t *threadParams = (threadParams_t *)threadp;
  int idleIdx = threadParams->threadIdx;

  do                                                                            // Calculates Fibonacci series.
  {
    FIB_TEST(seqIterations, Iterations);
    idleCount[idleIdx]++;
  } while(idleCount[idleIdx] < runInterference);

  gettimeofday(&timeNow, (void *)0);                                            // Displays time-stamp
  printf("**** %d idle NO SEM stopping at %d sec, %d nsec\n", idleIdx, (int)timeNow.tv_sec, (int)timeNow.tv_nsec);

  pthread_exit(NULL);
}


int CScnt=0;
/*******************************************************************************
 @brief: This call back function belongs to Low and High priority task. 
 *       It requrires semaphores.
*******************************************************************************/
void *idle(void *threadp)
{
  struct timespec timeNow;
  unsigned int idx = 0, jdx = 1;
  volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;
  threadParams_t *threadParams = (threadParams_t *)threadp;
  int idleIdx = threadParams->threadIdx;

  pthread_mutex_lock(&msgSem);
  CScnt++;

  do                                                                            // Calculates Fibonacci series.
  {
    FIB_TEST(seqIterations, Iterations);
    idleCount[idleIdx]++;
  } while(idleCount[idleIdx] < runInterference);

  sleep(2);

  idleCount[idleIdx]=0;

  do
  {
    FIB_TEST(seqIterations, Iterations);
    idleCount[idleIdx]++;
  } while(idleCount[idleIdx] < runInterference);

  pthread_mutex_unlock(&msgSem);

  gettimeofday(&timeNow, (void *)0);                                            // Displays time-stamp
  printf("**** %d idle stopping at %d sec, %d nsec\n", idleIdx, (int)timeNow.tv_sec, (int)timeNow.tv_nsec);

  pthread_exit(NULL);
}

/*******************************************************************************
*@brief: Dispays the Scheduling Policy.
*******************************************************************************/
void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
	   printf("Pthread Policy is SCHED_FIFO\n");
	   break;
     case SCHED_OTHER:
	   printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
	   printf("Pthread Policy is SCHED_OTHER\n");
	   break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}

int main (int argc, char *argv[])
{
   int rc, invSafe=0, i, scope;
   struct timespec sleepTime, dTime;

   CScount=0;

   if(argc < 2)
   {
     printf("Usage: pthread interfere-seconds\n");
     exit(-1);
   }
   else if(argc >= 2)                                                           // user arguments must be >2 for program to run. At <2 system cannot be scheduled.
   {
     sscanf(argv[1], "%d", &intfTime);
     printf("interference time = %d secs\n", intfTime);
     printf("unsafe mutex will be created\n");
   }

   print_scheduler();

   pthread_attr_init(&rt_sched_attr[START_SERVICE]);                            // Set up attributes for setting priroity vi FIFO.
   pthread_attr_setinheritsched(&rt_sched_attr[START_SERVICE], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
   pthread_attr_setschedpolicy(&rt_sched_attr[START_SERVICE], SCHED_FIFO);      // Scheduling is set as FIFO.

   pthread_attr_init(&rt_sched_attr[HIGH_PRIO_SERVICE]);                        // Set up attributes for setting priroity vi FIFO.
   pthread_attr_setinheritsched(&rt_sched_attr[HIGH_PRIO_SERVICE], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
   pthread_attr_setschedpolicy(&rt_sched_attr[HIGH_PRIO_SERVICE], SCHED_FIFO);  // Scheduling is set as FIFO.

   pthread_attr_init(&rt_sched_attr[MID_PRIO_SERVICE]);                         // Set up attributes for setting priroity vi FIFO.
   pthread_attr_setinheritsched(&rt_sched_attr[MID_PRIO_SERVICE], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
   pthread_attr_setschedpolicy(&rt_sched_attr[MID_PRIO_SERVICE], SCHED_FIFO);   // Scheduling is set as FIFO.

   pthread_attr_init(&rt_sched_attr[LOW_PRIO_SERVICE]);                         // Set up attributes for setting priroity vi FIFO.
   pthread_attr_setinheritsched(&rt_sched_attr[LOW_PRIO_SERVICE], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
   pthread_attr_setschedpolicy(&rt_sched_attr[LOW_PRIO_SERVICE], SCHED_FIFO);   // Scheduling is set as FIFO.

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);                            // Stores the highest and lowest priorities. 
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   rc=sched_getparam(getpid(), &nrt_param);

   if (rc) 
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   print_scheduler();

   printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);
   pthread_attr_getscope(&rt_sched_attr[START_SERVICE], &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)                                            // Displays Scope of the thread.
     printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
     printf("PTHREAD SCOPE PROCESS\n");
   else
     printf("PTHREAD SCOPE UNKNOWN\n");

   pthread_mutex_init(&msgSem, NULL);                                           // Initializes the Mutex.

   rt_param[START_SERVICE].sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&rt_sched_attr[START_SERVICE], &rt_param[START_SERVICE]);

   printf("Creating thread %d\n", START_SERVICE);                               // Creates the start service thread.
   threadParams[START_SERVICE].threadIdx=START_SERVICE;
   rc = pthread_create(&threads[START_SERVICE], &rt_sched_attr[START_SERVICE], startService, (void *)&threadParams[START_SERVICE]);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   printf("Start services thread spawned\n");


   printf("will join service threads\n");

   if(pthread_join(threads[START_SERVICE], NULL) == 0)                          // Terminates the start service thread.
     printf("START SERVICE done\n");
   else
     perror("START SERVICE");


   rc=sched_setscheduler(getpid(), SCHED_OTHER, &nrt_param);                    // Scheduler is set as OTHER.

   if(pthread_mutex_destroy(&msgSem) != 0)
     perror("mutex destroy");

   printf("All done\n");

   exit(0);
}

/*******************************************************************************
*@brief: Scheduler thread which initializes Low,Med and High priority threads.
*******************************************************************************/
void *startService(void *threadid)
{
   struct timespec timeNow;
   int rc;

   runInterference=intfTime;

   rt_param[LOW_PRIO_SERVICE].sched_priority = rt_max_prio-20;                  // Sets lowest priority for Low task.
   pthread_attr_setschedparam(&rt_sched_attr[LOW_PRIO_SERVICE], &rt_param[LOW_PRIO_SERVICE]); 

   printf("Creating thread %d\n", LOW_PRIO_SERVICE);
   threadParams[LOW_PRIO_SERVICE].threadIdx=LOW_PRIO_SERVICE;                   // Creates the thread for Low priority task.
   rc = pthread_create(&threads[LOW_PRIO_SERVICE], &rt_sched_attr[LOW_PRIO_SERVICE], idle, (void *)&threadParams[LOW_PRIO_SERVICE]);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[LOW_PRIO_SERVICE]);
   gettimeofday(&timeNow, (void *)0);                                           // Gets the current time.
   printf("Low prio %d thread spawned at %d sec, %d nsec\n", LOW_PRIO_SERVICE, (int)timeNow.tv_sec, (int)timeNow.tv_nsec);


   sleep(1);

   rt_param[MID_PRIO_SERVICE].sched_priority = rt_max_prio-10;                  // Sets lowest priority for Low task.
   pthread_attr_setschedparam(&rt_sched_attr[MID_PRIO_SERVICE], &rt_param[MID_PRIO_SERVICE]);

   printf("Creating thread %d\n", MID_PRIO_SERVICE);                            // Creates the thread for Low priority task.
   threadParams[MID_PRIO_SERVICE].threadIdx=MID_PRIO_SERVICE;
   rc = pthread_create(&threads[MID_PRIO_SERVICE], &rt_sched_attr[MID_PRIO_SERVICE], idleNoSem, (void *)&threadParams[MID_PRIO_SERVICE]);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[MID_PRIO_SERVICE]);
   gettimeofday(&timeNow, (void *)0);                                           // Gets the current time.
   printf("Middle prio %d thread spawned at %d sec, %d nsec\n", MID_PRIO_SERVICE, (int)timeNow.tv_sec, (int)timeNow.tv_nsec);

   rt_param[HIGH_PRIO_SERVICE].sched_priority = rt_max_prio-1;
   pthread_attr_setschedparam(&rt_sched_attr[HIGH_PRIO_SERVICE], &rt_param[HIGH_PRIO_SERVICE]);


   printf("Creating thread %d, CScnt=%d\n", HIGH_PRIO_SERVICE, CScnt);          // Creates the thread for Low priority task.
   threadParams[HIGH_PRIO_SERVICE].threadIdx=HIGH_PRIO_SERVICE;
   rc = pthread_create(&threads[HIGH_PRIO_SERVICE], &rt_sched_attr[HIGH_PRIO_SERVICE], idle, (void *)&threadParams[HIGH_PRIO_SERVICE]);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[HIGH_PRIO_SERVICE]);
   gettimeofday(&timeNow, (void *)0);                                           // Gets the current time.
   printf("High prio %d thread spawned at %d sec, %d nsec\n", HIGH_PRIO_SERVICE, (int)timeNow.tv_sec, (int)timeNow.tv_nsec);


   if(pthread_join(threads[LOW_PRIO_SERVICE], NULL) == 0)                       // Threads are terminated in increasing priority.
     printf("LOW PRIO done\n");
   else
     perror("LOW PRIO");

   if(pthread_join(threads[MID_PRIO_SERVICE], NULL) == 0)
     printf("MID PRIO done\n");
   else
     perror("MID PRIO");

   if(pthread_join(threads[HIGH_PRIO_SERVICE], NULL) == 0)
     printf("HIGH PRIO done\n");
   else
     perror("HIGH PRIO");


   pthread_exit(NULL);

}