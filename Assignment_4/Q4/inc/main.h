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

#include <linux/videodev2.h>

#include <time.h>
//#define MULTI_MODE	(5)                                                                         // Uncomment to provide all 5 resolutions in the output.
#define default_res	(2)
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define COLOR_CONVERT

#define HRES_STR "960"
#define VRES_STR "720"

#define H160   "160"
#define H320  "320"
#define H640  "640"
#define H800  "800"
#define H960 "960"

#define V120   "120"
#define V240  "240"
#define V480  "480"
#define V600  "600"
#define V720  "720"

#define nano (int)(1000000000)
#define micro (float)(0.000001)

enum io_method
{
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

void SyslogInit(void);
int InitDevice(void);
int OpenDevice(void);
int StartCapturing(void);
int mainloop(void);
int StopCapturing(void);
int UninitDevice(void);
int CloseDevice(void);
void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);
float TimeValues(void);
int YUYV2GREY(unsigned char* pptr,int size);
int i;
// Format is used by a number of functions, so made as a file global
struct v4l2_format fmt;
struct timespec start,finish,difference;

struct buffer 
{
        void   *start;
        size_t  length;
};

