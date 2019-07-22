#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <syslog.h>
/*****************************************************************************
@brief: Intercommunication mechanism.
============================================================================*/
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

#include <linux/videodev2.h>

#include <time.h>
#define MULTI_MODE	(5)                                                                         // Uncomment to provide all 5 resolutions in the output.
#define default_res	(1)
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define COLOR_CONVERT

#define HRES_STR "960"
#define VRES_STR "720"

#define H160  "160"
#define H320  "320"
#define H640  "640"
#define H800  "800"
#define H960  "960"

#define V120  "120"
#define V240  "240"
#define V480  "480"
#define V600  "600"
#define V720  "720"

#define nano (int)(1000000000)
#define micro (float)(0.001)
#define milli (float)(0.000001)

#define TotalFrames	(30)

enum io_method
{
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

void SyslogInit(void);
int InitDevice(void);
int OpenDevice(void);
int UninitDevice(void);
int CloseDevice(void);
void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);
void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);
float TimeValues(void);
float FrameRateCal(void);
float DeadlineCal(void);
int YUYV2GREY(unsigned char* pptr,int size);
int BRIGHT(unsigned char* pptr,int size);
int CONTRAST(unsigned char* pptr,int size);
int InitThreads(void);
void print_scheduler(void);
void DisplayDeadlines(void);
float wcet(int);
float bcet(int);
float average(int);
int i;
// Format is used by a number of functions, so made as a file global
float frame_rate, deadline;
int deadline_index;
struct v4l2_format fmt;
struct timespec start,finish,difference,start_f,frame_difference,stop,start_d,stop_d,dead_difference;
int CONVERSION_METHOD;

struct buffer 
{
        void   *start;
        size_t  length;
};

/***************************************************************************
@ brief: Inter thread communication mechanisms.
***************************************************************************/
pthread_mutex_t MUTEX; 
mqd_t mymq;
sem_t semF10, semF20;
int t1;
int t2;
pthread_t threads[3];
void* (*function_pointer[3])(void);
cpu_set_t threadcpu;
cpu_set_t allcpuset;
pid_t mainpid;
#define NUM_THREADS		(3)
#define NUM_CPU_CORES	(1)
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
typedef struct
{
    int threadIdx;
    int MajorPeriods;
} threadParams_t;
threadParams_t threadParams[NUM_THREADS];
int rc;
enum v4l2_buf_type type;
/*****************************************************************************
@brief: ProcessImagePPM
*****************************************************************************/
int i, newi, newsize;
struct timespec frame_time;

/*****************************************************************************
@brief: ReadThread
*****************************************************************************/
struct v4l2_buffer buf;
unsigned int count;
struct timespec read_delay;
struct timespec time_error;
unsigned int j;
int FRAME_COUNT;

int PROCESS_COMPLETE_BIT;
int resolution;
int newi;

