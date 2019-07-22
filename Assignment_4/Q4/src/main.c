/*
 *
 *  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  grabber NTSC cameras to acquire digital video from a source,
 *  time-stamp each frame acquired, save to a PGM or PPM file.
 *
 *  The original code adapted was open source from V4L2 API and had the
 *  following use and incorporation policy:
 * 
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 * Similar Code: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html?highlight=fd_set
 */

#include "main.h"

int HRES[5]={160,320,640,800,960};
int VRES[5]={120,240,480,600,720};

char* dev_name;
enum io_method io = IO_METHOD_MMAP;
int fd = -1;
struct buffer *buffers;
unsigned int n_buffers;
int out_buf;
int force_format=1;
int frame_count = 30;
unsigned char gray_ptr[(1280*960)];

char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_dumpname[]="test00000000.pgm";
unsigned int framecnt=0;

unsigned int pgm_header_len = sizeof(pgm_header);

int main(void)
{
    i=default_res;
    clock_gettime(CLOCK_REALTIME, &start);
    SyslogInit();
#if defined(MULTI_MODE)
 for(i=0;i<MULTI_MODE;i+=1)
 {
#endif
    if(OpenDevice()==-1)
    {
        exit(1);
    }
    if(InitDevice()==-1)
    {
        exit(1);
    }
    if(StartCapturing()==-1)
    {
        exit(1);
    }
    if(mainloop()==-1)
    {
        exit(1);
    }
    if(StopCapturing()==-1)
    {
        exit(1);
    }
    if(UninitDevice()==-1)
    {
        exit(1);
    }
    if(CloseDevice()==-1)
    {
        exit(1);
    }
    syslog(LOG_INFO,"[TIME:%fus]Completed Reading frame", TimeValues());
#if defined(MULTI_MODE)
syslog(LOG_INFO,"[TIME:%fus]Completed Reading frame: %d times",TimeValues(),(i+1));
}
#endif
    printf("In order to see the syslog, type the following: cat /var/log/syslog | grep 'capture'\n");
    return 0;
}

