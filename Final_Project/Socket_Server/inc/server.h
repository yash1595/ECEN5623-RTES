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

#include <time.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/socket.h> 

#define PORT 			(8080)
#define BUFF_SIZE 		(153600)
#define nano 			(int)(1000000000)
#define TOTAL_FRAMES		(50)
#define IMG_SIZE		((uint32_t)(307200))


struct timespec finish,start,difference;

#define HRES_STR 	"640"
#define VRES_STR 	"480"

#define H640  		"640"
#define V480  		"480"
#define INDEX_1		(75)
#define INDEX_2		(4096)

//variables
int sock, mysock, valread;

struct sockaddr_in server;
pthread_t capture_frames;

unsigned char receive_buffer[BUFF_SIZE];
char pgm_dumpname_grey[50];
char pgm_header[50];
unsigned int pgm_header_len;
int written,total, dumpfd, queue_bytes;
int send_fd;
unsigned char split_array[IMG_SIZE];
struct timespec frame_conv_time;
pid_t mainpid;
int rt_max_prio, rt_min_prio;
int rc;
cpu_set_t threadcpu;
pthread_attr_t rt_sched_attr[1];
struct sched_param rt_param[1];
struct sched_param main_param;

uint8_t PROCESS_COMPLETE_BIT;

float TimeValues(void);
void SyslogInit(void);
int SocketInit(void);
int HeadersInit(void);
int ConnectToClient(void);
void* ReadFrame(void);
void print_scheduler(void);
