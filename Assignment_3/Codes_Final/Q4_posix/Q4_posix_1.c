// Sam Siewert, September 2016
//
// Check to ensure all your CPU cores on in an online state.
//
// Check /sys/devices/system/cpu or do lscpu.
//
// Tegra is normally configured to hot-plug CPU cores, so to make all available,
// as root do:
//
// echo 0 > /sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/enable
// echo 1 > /sys/devices/system/cpu/cpu1/online
// echo 1 > /sys/devices/system/cpu/cpu2/online
// echo 1 > /sys/devices/system/cpu/cpu3/online
//
// Check for precision time resolution and support with cat /proc/timer_list
//
// Ideally all printf calls should be eliminated as they can interfere with
// timing.  They should be replaced with an in-memory event logger or at least
// calls to syslog.

// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <mqueue.h>

#define USEC_PER_MSEC (1000)
#define NUM_CPU_CORES (1)
#define FIB_TEST_CYCLES (100)
#define NUM_THREADS (2)     // service threads + sequencer
sem_t semF10, semF20;

#define FIB_LIMIT_FOR_32_BIT (47)
#define FIB_LIMIT (10)

static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";

pthread_t idle_thr_id;
static int idle_tid;
struct mq_attr mq_attr;
static char *shared_buffer; /* pointer for
Unix shared memory */

#define ERROR -1

#define NUMSIGS 10
#define TSIGRTMIN  SIGRTMIN+1

#define SNDRCV_MQ "/mqt"                                                        // Changed the queue name to include "/" as per mqueue.h
#define MAX_MSG_SIZE 128

#define MAX_MSG_SIZE 128

int abortTest = 0;
double start_time;

unsigned int seqIterations = FIB_LIMIT;
unsigned int idx = 0, jdx = 1;
unsigned int fib = 0, fib0 = 0, fib1 = 1;

double getTimeMsec(void);




typedef struct
{
    int threadIdx;
    int MajorPeriods;
} threadParams_t;


void print_scheduler(void)                                                      // Displays the scheduling policy.
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }

}

/*******************************************************************************
@brief: Sender sends messages via a queue directly without mallocing as in heap.
*******************************************************************************/
void sender(void)
{

  mqd_t mymq;                                                                   // Message queue object.
  int prio;
  int nbytes;

  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  if(mymq < 0)
  {
    perror("sender mq_open");
    exit(-1);
  }
  else
  {
    printf("sender opened mq\n");
  }

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)     // Message sent via queue.
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
}

/*******************************************************************************
@brief: Receiver receives messages via queue.
*******************************************************************************/
void receiver(void)
{
  mqd_t mymq;
  char buffer[MAX_MSG_SIZE];
  int prio;
  int nbytes;

  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  if(mymq == (mqd_t)ERROR)
  {
    perror("receiver mq_open");
    exit(-1);
  }

  /* read oldest, highest priority msg from the message queue */
  if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR)         // The message is received in a local array called buffer.
  {
    perror("mq_receive");
  }
  else
  {
    buffer[nbytes] = '\0';
    printf("receive: msg %s received with priority = %d, length = %d\n",        // Displays the received message and length.
           buffer, prio, nbytes);
  }
    
}

void main(void)
{
    int i, rc, scope;
    cpu_set_t threadcpu;
    pthread_t threads[NUM_THREADS];
    threadParams_t threadParams[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;
    cpu_set_t allcpuset;

    mq_attr.mq_maxmsg = 100;
    mq_attr.mq_msgsize = MAX_MSG_SIZE;

    mq_attr.mq_flags = 0;

    abortTest=0;

   printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());  //Displays the processors and those available.

   CPU_ZERO(&allcpuset);

   for(i=0; i < NUM_CPU_CORES; i++)
       CPU_SET(i, &allcpuset);

   printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));      // Displays which CPUs are being used.

    mainpid=getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);                           // Priorities for the tasks (max,min)
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rt_max_prio = 30;

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);                   // Scheduler is set to FIFO.
    if(rc < 0) perror("main_param");
    print_scheduler();


    pthread_attr_getscope(&main_attr, &scope);

    if(scope == PTHREAD_SCOPE_SYSTEM)                                           // Scope of the system is dislayed.
      printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
      printf("PTHREAD SCOPE PROCESS\n");
    else
      printf("PTHREAD SCOPE UNKNOWN\n");

    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);

    for(i=0; i < NUM_THREADS; i++)
    {

      CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
      CPU_SET(3, &threadcpu);                                                   // for each thread.


      rc=pthread_attr_init(&rt_sched_attr[i]);                                  // Set up attributes for setting priroity vi FIFO.
      rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
      rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);            // Scheduling is set as FIFO.
      rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

      rt_param[i].sched_priority=rt_max_prio-i;
      pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

      threadParams[i].threadIdx=i;
    }
   
    printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));//Displays which core the threada will run-on.

    // Create Service threads which will block awaiting release for:
    //
    // serviceF10
    rc=pthread_create(&threads[0],               // pointer to thread descriptor
                      0,//&rt_sched_attr[0],         // use specific attributes
                      //(void *)0,                 // default attributes
                      sender,                     // thread function entry point
                      NULL // parameters to pass in
                     );
    // serviceF20
    usleep(300000);

    rc=pthread_create(&threads[1],               // pointer to thread descriptor
                      0,//&rt_sched_attr[2],         // use specific attributes
                      //(void *)0,                 // default attributes
                      receiver,                     // thread function entry point
                      NULL // parameters to pass in
                     );


    // Wait for service threads to calibrate and await relese by sequencer
    


   for(i=0;i<NUM_THREADS;i++)                                                   // Terminate the threads.
       pthread_join(threads[i], NULL);

   mq_close(SNDRCV_MQ);                                                         // Closes the queue.
   mq_unlink(SNDRCV_MQ);                                                        // Unlinks the queue
   printf("\nTEST COMPLETE\n");

}
             
