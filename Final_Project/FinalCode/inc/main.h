#ifndef __MAIN_H__
#define __MAIN_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

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
#include <mqueue.h>
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

#define HRES_STR "640"
#define VRES_STR "480"
#define H640  "640"
#define V480  "480"

#define nano		(int)(1000000000)
#define DecNano 	(float)(0.000000001)
#define divide_by_50ms  (int)(5)
#define divide_by_100ms (int)(10)
#define divide_by_1s    (int)(100)
/*************************************************************************************/
#define true (1)
#define false (0)
uint8_t SocketSet;
int CAPTURE_RATE;

#define SOCKET	"socket"
#define _10HZ_	"10HZ"
#define _1s_    (1)
#define _600ms_ (600000000)
#define _500ms_ (500000000)
#define _490ms_ (490000000)
#define _100ms_	(100000000)
#define _10ms_  (10000000)
#define _20ms_  (20000000)
#define _30ms_  (30000000)
#define _40ms_  (40000000)
#define _60ms_  (60000000)
#define _50ms_  (50000000)
#define _5ms_	(5000000)

#define TotalFrames 		(100)

#define SCHED_BITMASK		(uint8_t)(0x03)
#define GREY_BITMASK		(uint8_t)(0X01)
#define READ_BITMASK		(uint8_t)(0X02)
#define _10HZ_MODE		(uint8_t)(10)
#define _1HZ_MODE		(uint8_t)(1)
#define SKIP_FRAMES		(uint8_t)(150)
#define IMG_SIZE		(uint32_t)(307200)
#define lower_limit_thresh	(int)(-12)
#define upper_limit_thresh	(int)(30)
#define shifted_frame_thresh	(uint32_t)(800)
#define max_sched_frames	(uint32_t)(200000)

uint8_t TASK_BITMASK;
uint32_t OldframeCount;
int TotalFrameCount;

int ReadFrameCount;

struct COMMON_IMG_PROC
{
   const void*  ptr;
   int frame_count;
   char conv_type;
   int SIZE;
   int unique_id;
   struct timespec* frame_conv_time;
};

struct COMMON_IMG_PROC CIRCULAR_BUFFER[100];
int circ_read,circ_write;

struct COMMON_IMG_PROC common_struct;


/*************************************************************************************/

// Format is used by a number of functions, so made as a file global
float frame_rate, deadline;
int deadline_index;
struct v4l2_format fmt;
struct timespec start,finish,difference,start_f,frame_difference,stop,start_d,stop_d,dead_difference, Readreq, Readrem, Dumpreq, Dumprem;

int CONVERSION_METHOD;

struct buffer 
{
        void   *start;
        size_t  length;
};

/***************************************************************************
@ brief: Inter thread communication mechanisms.
***************************************************************************/

sem_t semRead, semGray;

#define NUM_THREADS		(5)
#define NUM_CPU_CORES		(3)

pthread_t threads[NUM_THREADS];
void* (*function_pointer[NUM_THREADS])(void);
cpu_set_t threadcpu;
cpu_set_t allcpuset;
pid_t mainpid;


pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;

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
unsigned int j;
int FRAME_COUNT;

int PROCESS_COMPLETE_BIT;
int resolution;

#define TEST_FRAMES	(10)
struct COMMON_IMG_PROC COMM_ARRAY[TEST_FRAMES];
struct timespec stop_time;
int common_struct_index;
int diff_sec, diff_nsec;

struct timespec start_pthread_grey, stop_pthread_grey;
struct timespec start_read_frame, stop_read_frame;
struct timespec start_dump_pgm, stop_dump_pgm;
struct timespec start_scheduler, stop_scheduler;

/************************************************************************************/
int              fd;
struct buffer          *buffers;
unsigned int     n_buffers;
int              frame_count;

unsigned char gray_ptr[(1280*960)];

#define GREY		(0)
#define READ		(1)
#define DUMP		(2)
#define SCHEDULER	(3)

#define START	(0)
#define STOP	(1)

typedef struct 
{
	 int frame_index;
	 float grey_thread[TotalFrames][2];
	 float read_thread[TotalFrames][2];
	 float dump_thread[TotalFrames][2];
	 float read_time[TotalFrames];
         float grey_time[TotalFrames];
         float dump_time[TotalFrames];
	 //float distribution_count[][Total];
}exectimes;

exectimes exec_time;
float sched_time[max_sched_frames];
float sched_thread[max_sched_frames][2];
int independent_sched;
int independent;
unsigned int framecnt;
float sum;

int HRES;
int VRES;
char* dev_name;
unsigned int pgm_header_len;
void print_scheduler(void);
int xioctl(int fh, int request, void *arg);

#endif
