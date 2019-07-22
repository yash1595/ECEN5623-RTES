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
 * https://gist.github.com/maxlapshin/1253534
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <time.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define COLOR_CONVERT
#define HRES 320
#define VRES 240
#define HRES_STR "320"
#define VRES_STR "240"

void rgb2gray(unsigned char*,int);
// Format is used by a number of functions, so made as a file global
static struct v4l2_format fmt;

enum io_method 
{
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR, 
};

struct buffer 
{
        void   *start;
        size_t  length;
};

static char            *dev_name;
//static enum io_method   io = IO_METHOD_USERPTR;
//static enum io_method   io = IO_METHOD_READ;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format=1;
static int              frame_count = 30;

/*******************************************************************************
*@brief: Prints the error into stderr file pointer and then exits via 
*        exit(EXIT_FAILURE).
*@param: The error string is passed from the function where error occured.
*******************************************************************************/
static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

/*******************************************************************************
*@Link: http://man7.org/linux/man-pages/man2/ioctl.2.html.
================================================================================
*@brief: Recursively runs 'ioctal' till error value is not returned.
*@param: 1. fh - File pointer.
*        2. request - 
*        3. arg - struct v4l2_buffer*.
*@return: Value which is interpreted by the calling function.
*******************************************************************************/
static int xioctl(int fh, int request, void *arg)
{
        int r;

        do 
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
}

char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n"; 
char ppm_dumpname[]="test00000000.ppm";
char pgm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n"; 
char pgm_dumpname[]="test00000000.pgm";

/*******************************************************************************
@brief : Generates an image name and saves it to the host device.
@params: p - UNsigned character pointer with pixel values.
*        size - Number of bytes to write.
*        tag - frame count
*        timespec *time - Structure to store time stamp.
*******************************************************************************/
static void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;                                              
   
    snprintf(&ppm_dumpname[4], 9, "%08d", tag);                                 // Stores the frame count in ppm_dumpname.
    strncat(&ppm_dumpname[12], ".ppm", 5);                                      // Appends the .ppm extension to the image name.
    dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);        // Opens the ppm_dumpname with dumpfd file descriptor. 

    snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);                   // Adds the time-stamp to ppm_header.
    strncat(&ppm_header[14], " sec ", 5);                                       // Appends the "sec" to the time-stamp.
    snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));     // Appends the actual time in nsec to the time-stamp.
    strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);       // Appends the msec values.
    written=write(dumpfd, ppm_header, sizeof(ppm_header));                      // Writes the above mentioned values to file descriptor dumpfd.

    total=0;

    do                                                                          // Recursively write to dumpfd for all values of size.
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    printf("wrote %d bytes\n", total);

    close(dumpfd);                                                              // Closes the file descriptor. 
    
}

/*******************************************************************************
@brief : Generates an image name and saves it to the host device.
@params: p - UNsigned character pointer with pixel values.
*        size - Number of bytes to write.
*        tag - frame count
*        timespec *time - Structure to store time stamp.
*******************************************************************************/
static void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;
   
    snprintf(&pgm_dumpname[4], 9, "%08d", tag);                                 // Stores the frame count in ppm_dumpname.
    strncat(&pgm_dumpname[12], ".pgm", 5);                                      // Appends the .pgm extension to the image name.
    dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);        // Opens the ppm_dumpname with dumpfd file descriptor. 

    snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);                   // Adds the time-stamp to ppm_header
    strncat(&pgm_header[14], " sec ", 5);                                       // Appends the "sec" to the time-stamp.
    snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));     // Appends the actual time in nsec to the time-stamp.
    strncat(&pgm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);       // Appends the msec values.
    written=write(dumpfd, pgm_header, sizeof(pgm_header));                      // Writes the above mentioned values to file descriptor dumpfd.

    total=0;

    do                                                                          // Recursively write to dumpfd for all values of size.
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    printf("wrote %d bytes\n", total);

    close(dumpfd);                                                              // Closes the file descriptor. 
    
}

/*******************************************************************************
@brief : Converts the values passed as parameters into r,g and b float values.
@params: y,u,v - Passed values of RGB in float.
*        *r,*g and *b - Saves the resultant unsigned character values. 
*******************************************************************************/
void yuv2rgb_float(float y, float u, float v, 
                   unsigned char *r, unsigned char *g, unsigned char *b)
{
    float r_temp, g_temp, b_temp;

    // R = 1.164(Y-16) + 1.1596(V-128)
    r_temp = 1.164*(y-16.0) + 1.1596*(v-128.0);  
    *r = r_temp > 255.0 ? 255 : (r_temp < 0.0 ? 0 : (unsigned char)r_temp);     // If r_temp > 255 it is converted to 255. If r_temp < 0 it is converted to 0.

    // G = 1.164(Y-16) - 0.813*(V-128) - 0.391*(U-128)
    g_temp = 1.164*(y-16.0) - 0.813*(v-128.0) - 0.391*(u-128.0);
    *g = g_temp > 255.0 ? 255 : (g_temp < 0.0 ? 0 : (unsigned char)g_temp);     // If g_temp > 255 it is converted to 255. If r_temp < 0 it is converted to 0.

    // B = 1.164*(Y-16) + 2.018*(U-128)
    b_temp = 1.164*(y-16.0) + 2.018*(u-128.0);
    *b = b_temp > 255.0 ? 255 : (b_temp < 0.0 ? 0 : (unsigned char)b_temp);     // If b_temp > 255 it is converted to 255. If r_temp < 0 it is converted to 0.
}


// This is probably the most acceptable conversion from camera YUYV to RGB
//
// Wikipedia has a good discussion on the details of various conversions and cites good references:
// http://en.wikipedia.org/wiki/YUV
//
// Also http://www.fourcc.org/yuv.php
//
// What's not clear without knowing more about the camera in question is how often U & V are sampled compared
// to Y.
//
// E.g. YUV444, which is equivalent to RGB, where both require 3 bytes for each pixel
//      YUV422, which we assume here, where there are 2 bytes for each pixel, with two Y samples for one U & V,
//              or as the name implies, 4Y and 2 UV pairs
//      YUV420, where for every 4 Ys, there is a single UV pair, 1.5 bytes for each pixel or 36 bytes for 24 pixels

/*******************************************************************************
@brief: Converts the values passed as parameters into r,g and b float values.
@params: y,u,v - Passed values of RGB in integer.
*        *r,*g and *b - Saves the resultant unsigned character values. 
*******************************************************************************/
void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
   int r1, g1, b1;

   // replaces floating point coefficients
   int c = y-16, d = u - 128, e = v - 128;       

   // Conversion that avoids floating point                                     // Method described on wikipedia.
   r1 = (298 * c           + 409 * e + 128) >> 8;
   g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
   b1 = (298 * c + 516 * d           + 128) >> 8;

   // Computed values may need clipping.
   if (r1 > 255) r1 = 255;                                                      // If b_temp > 255 it is converted to 255.
   if (g1 > 255) g1 = 255;                                                      // If b_temp > 255 it is converted to 255. 
   if (b1 > 255) b1 = 255;                                                      // If b_temp > 255 it is converted to 255. 

   if (r1 < 0) r1 = 0;                                                          // If r_temp < 0 it is converted to 0.
   if (g1 < 0) g1 = 0;                                                          // If r_temp < 0 it is converted to 0.
   if (b1 < 0) b1 = 0;                                                          // If r_temp < 0 it is converted to 0.

   *r = r1 ;
   *g = g1 ;
   *b = b1 ;
}

unsigned int framecnt=0;
unsigned char bigbuffer[(1280*960)];

/*******************************************************************************
@brief : Stores the images into .ppm or .pgm format. based on the values set in
*        fmt.pixelformat.
@params: p    - Unsigned character pointer
*        size - Number of pixels.
*******************************************************************************/
static void process_image(const void *p, int size)
{
    int i, newi, newsize=0;
    struct timespec frame_time;
    int y_temp, y2_temp, u_temp, v_temp;
    unsigned char *pptr = (unsigned char *)p;

    // record when process was called
    clock_gettime(CLOCK_REALTIME, &frame_time);                                 // Gets the current time.

    framecnt++;                                                                 // Increments the framecnt everytime a new frame is captured.
    printf("frame %d: ", framecnt);

    // This just dumps the frame to a file now, but you could replace with whatever image
    // processing you wish.
    //

    if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)                            // If format is grey, dump_pgm is called and image is saved as .pgm.
    {
        printf("Dump graymap as-is size %d\n", size);
        dump_pgm(p, size, framecnt, &frame_time);
    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)                       // For YUYV formats.
    {

#if defined(COLOR_CONVERT)
        printf("Dump YUYV converted to RGB size %d\n", size);
       
        // Pixels are YU and YV alternating, so YUYV which is 4 bytes
        // We want RGB, so RGBRGB which is 6 bytes
        //
        for(i=0, newi=0; i<size; i=i+4, newi=newi+6)                            // Stores the YUYV values into bigbuffer as RGB values. 
        {
            y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; 
            v_temp=(int)pptr[i+3];
            yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1],\
                    &bigbuffer[newi+2]);
            yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], \
                    &bigbuffer[newi+4], &bigbuffer[newi+5]);
        }
	printf("Newi = %d bigbuffer:%d size = %d\n",newi,sizeof(bigbuffer)/sizeof(unsigned char),(size*6)/4);
	
    dump_ppm(bigbuffer,(size*6/4), framecnt, &frame_time);                      // Saves image as .pgm.
#else
        printf("Dump YUYV converted to YY size %d\n", size);                    // If #define(COLOR_CONVERT) is not present, YUYV is converted to YY format. 
       
        // Pixels are YU and YV alternating, so YUYV which is 4 bytes
        // We want Y, so YY which is 2 bytes
        //
        for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
        {
            // Y1=first byte and Y2=third byte
            bigbuffer[newi]=pptr[i];
            bigbuffer[newi+1]=pptr[i+2];
        }
        printf("size/2 : %d\n",(size/2));
        dump_pgm(bigbuffer, (size/2), framecnt, &frame_time);                   // Saves image as .pgm.
#endif

    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24)                      // If image was captured in RGB24 format, it will be converted to .ppm format.
    {
        printf("Dump RGB as-is size %d\n", size);
        dump_ppm(p, size, framecnt, &frame_time);                               // Saves image as .ppm.
    }
    else
    {
        printf("ERROR - unknown dump format\n");
    }

    fflush(stderr);                                                             // Clears the stderr buffer.
    //fprintf(stderr, ".");
    fflush(stdout);
}

/*******************************************************************************
@brief: Reads a frame at a time and then processess each frame storing it an 
* individual image.
*******************************************************************************/
static int read_frame(void)
{
    struct v4l2_buffer buf;
    unsigned int i;

    switch (io)
    {

        case IO_METHOD_READ:                                                    // Reads the buffer and returns an error.
            if (-1 == read(fd, buffers[0].start, buffers[0].length))
            {
                switch (errno)
                {

                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("read");
                }
            }

        process_image(buffers[0].start, buffers[0].length);                     // Processes the 30 images.
        break;

        case IO_METHOD_MMAP:                                                    // Under the mmap 
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Type is set as enum value = 1.
            buf.memory = V4L2_MEMORY_MMAP;                                      // Based on memory mmap, read-io or user ptr.

            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))                           // https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/vidioc-qbuf.html?highlight=vidioc_dqbuf
            {                                                                   // Enqueues an empty/full buffer onto the drivers queue.
                switch (errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, but drivers should only set for serious errors, although some set for
                           non-fatal errors too.
                         */
                        return 0;


                    default:
                        printf("mmap failure\n");
                        errno_exit("VIDIOC_DQBUF");                             // Error value prints message and quits.
                }
            }

            assert(buf.index < n_buffers);
            process_image(buffers[buf.index].start, buf.bytesused);             // Processes the 30 images.

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");                                  // Error value prints message and quits.
            break;

        case IO_METHOD_USERPTR:
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Type is set as enum value = 1.
            buf.memory = V4L2_MEMORY_USERPTR;                                   // Based on memory mmap, read-io or user ptr.

            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))                           // Enqueues an empty/full buffer onto the drivers queue.
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:    
                        errno_exit("VIDIOC_DQBUF");                             // Error value prints message and quits.
                }
            }

            for (i = 0; i < n_buffers; ++i)
                    if (buf.m.userptr == (unsigned long)buffers[i].start
                        && buf.length == buffers[i].length)
                            break;

            assert(i < n_buffers);
            process_image((void *)buf.m.userptr, buf.bytesused);                // Processes the 30 images.

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");                                  // Error value prints message and quits.
            break;
    }

    //printf("R");
    return 1;
}


/*******************************************************************************
@brief: Takes 30 frames in a continuous loop. 
*******************************************************************************/
static void mainloop(void)
{
    unsigned int count;
    struct timespec read_delay;
    struct timespec time_error;

    read_delay.tv_sec=0;
    read_delay.tv_nsec=30000;                                                   // Delay values are set to 30us.

    count = frame_count;

    while (count > 0)
    {
        for (;;)
        {
            fd_set fds;                                                         // Predefined buffer. 
            struct timeval tv;
            int r;

            FD_ZERO(&fds);                                                      // Clears the fds structure.
            FD_SET(fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;                                                      // Maximum timeout limited up-to 2s.
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);                          // Waits for up to 2s before capturing an image.

            if (-1 == r)                                                        // Indicates failure
            {
                if (EINTR == errno)
                    continue;
                errno_exit("select");
            }

            if (0 == r)                                                         // Indicates timeout.
            {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

            if (read_frame())
            {
/*****************************************************************************
@Link : link: http://man7.org/linux/man-pages/man2/nanosleep.2.html
@brief: nanosleep() suspends the execution of the calling thread until either
        at least the time specified in *req has elapsed, or the delivery of a
        signal that triggers the invocation of a handler in the calling thread 
        or that terminates the process. 
@params: &read_delay - Pointer to time taken to execute. 
         &time_error - Stores the elapsed time in time_error.
*****************************************************************************/
                if(nanosleep(&read_delay, &time_error) != 0)
                    perror("nanosleep");
                else
                    printf("time_error.tv_sec=%ld, time_error.tv_nsec=%ld\n",time_error.tv_sec, time_error.tv_nsec);  // Displays the balance amount of seconds and nano-seconds left.
                                      

                count--;
                break;
            }

            /* EAGAIN - continue select loop unless count done. */
            if(count <= 0) break;                                               // A zero or negative value quits the loop.
        }

        if(count <= 0) break;                                                   // A zero or negative value quits the loop.
    }
}

/*******************************************************************************
@Link: //https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec/rn01re61.html
@brief: Ends the video capturing process.
*******************************************************************************/
static void stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Type is set as enum value = 1. 
                if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))                  // Stops the video streaming.
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }
}

/*******************************************************************************
@brief: Begins the video capture by loading the VIDIOC_QBUF queue.
*******************************************************************************/
static void start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io)   
        {

        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)                                 // Initially value is '0'.
                {
                        printf("allocated buffer %d\n", i);
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))                //loads the queue(enqueue) for incoming data.
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Type is loaded with Video Capture enum.
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))                   // Starts the capture process.
                        errno_exit("VIDIOC_STREAMON");                          // Error value prints message and quits.
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;                                 // https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/buffer.html#c.v4l2_buffer

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long)buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))                //loads the queue(enqueue) for incoming data.
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Starts the capture process.
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))                   // Begins streaming.
                        errno_exit("VIDIOC_STREAMON");                          // Error value prints message and quits.
                break;
        }
}

/*******************************************************************************
@brief: Frees all the buffers malloced by MMAP or User-Pointer.
*******************************************************************************/
static void uninit_device(void)
{
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                {
/*******************************************************************************
@brief: he munmap() function shall remove any mappings for those entire pages 
* containing any part of the address space of the process starting at addr and
* continuing for len bytes.
@params: Starting address of the buffers[] and length of the same buffer.
*******************************************************************************/                   
                    if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");                           // Error value prints message and quits.
                }                                
                        
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);                                 // Frees the buffer.
                break;
        }

        free(buffers);
}

/*******************************************************************************
@brief : Used for Read-Write operations.
@params: Buffer size to read.
*******************************************************************************/
static void init_read(unsigned int buffer_size)
{
        buffers = calloc(1, sizeof(*buffers));                                  // Initialize the buffers with all 0s.

        if (!buffers) 
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);                                             // Error value prints message and quits.
        }

        buffers[0].length = buffer_size;                                        // Stores the buffer size in .length field.
        buffers[0].start = malloc(buffer_size);                                 // Allocates the suitable size to .start.

        if (!buffers[0].start) 
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);                                             // Error value prints message and quits.
        }
}

/*******************************************************************************
@brief: Opted by Memroy Mapped I/O.
*******************************************************************************/
static void init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 6;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                 // Sets type to Video capture.
        req.memory = V4L2_MEMORY_MMAP;                                          // The memory is set as MMAP.

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))                             // Mmap buffers are allocated by ioctl first before being mapped by applications address place.
        {
                if (EINVAL == errno) 
                {
                        fprintf(stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit(EXIT_FAILURE);                                     // Error value prints message and quits.
                } else 
                {
                        errno_exit("VIDIOC_REQBUFS");                           // Error value prints message and quits.
                }
        }

        if (req.count < 2)                                                      // If counts are less than 2 end program.
        {
                fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
                exit(EXIT_FAILURE);                                             // Error value prints message and quits.
        }

        buffers = calloc(req.count, sizeof(*buffers));                          // Allocates 6 buffers with all 0s.

        if (!buffers) 
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);                                             // Error value prints message and quits.
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;                  // Sets type to Video capture.
                buf.memory      = V4L2_MEMORY_MMAP;                             // The memory is set as MMAP.
                buf.index       = n_buffers;                                    // buf.index is used to store the current n_buffers value. 

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");                          // Error value prints message and quits.

                buffers[n_buffers].length = buf.length;                         // Buf length is stored in each .length component of buffers[] structure array.
/*******************************************************************************
@Link: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/func-mmap.html?highlight=mmap
================================================================================
@brief  : Maps te device memory into the applications address space.
@params : start - 
*         length - Length of the buf.length.
*         prot - Describes the desired memory protection.
*         flags - Sets the flags for the operations.
*         fd -  The file descriptor.
*         offset - Offset of buffer in device memroy.
*******************************************************************************/
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,            // Pages may be read or written.
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)                     // Returns this value when mmap fails. MAP_FAILED = (void*)(-1)
                        errno_exit("mmap");                                     // Error value prints message and quits.
        }
}

/*******************************************************************************
@brief : Opted by User Pointer.
@params: The buffer size. 
*******************************************************************************/
static void init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;                               // Sets type to Video capture.
        req.memory = V4L2_MEMORY_USERPTR;                                       // The memory is set as USER POINTER.

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {                           // User space buffers are allocated by ioctl first before being mapped by applications address place.
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit(EXIT_FAILURE);                                     // Error value prints message and quits.
                } else {
                        errno_exit("VIDIOC_REQBUFS");                           // Error value prints message and quits.
                }
        }

        buffers = calloc(4, sizeof(*buffers));                                  // Allocates 4 buffers with all 0s.

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);                                             // Error value prints message and quits.
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;                        // Buf length is stored in each .length component of buffers[] structure array.
                buffers[n_buffers].start = malloc(buffer_size);                 // .start is a Union within the v4l2_requestbuffers buffer of the passed buffer size.

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);                                     // Error value prints message and quits.
                }
        }
}

/*******************************************************************************
@brief: Camera is checked for video streaming, cropping the image and capture 
* images in a specified format. In this case, YUYV.
*******************************************************************************/
static void init_device(void)
{
/*****************************************************************************
*  v4l2_capability Struct:
*   struct v4l2_rect    bounds
*   struct v4l2_rect    defrect
*   struct v4l2_fract   pixelaspect
*   enum v4l2_buf_type  type

*  v4l2_crop Struct:
*   struct v4l2_rect    c
*   enum v4l2_buf_type  type

 enum v4l2_buf_type {
     V4L2_BUF_TYPE_VIDEO_CAPTURE  = 1,
     V4L2_BUF_TYPE_VIDEO_OUTPUT   = 2,
     V4L2_BUF_TYPE_VIDEO_OVERLAY  = 3,
     V4L2_BUF_TYPE_VBI_CAPTURE    = 4,
     V4L2_BUF_TYPE_VBI_OUTPUT     = 5,
     V4L2_BUF_TYPE_PRIVATE        = 0x80
 };

 * v4l2_capability Struct:
*    __u8    driver [16]
*    __u32   card [32]
*    __u8    bus_info [32]
*    __u32   version
*    __u32   device_caps
*    __u32   reserved [3]
*******************************************************************************/
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))                                // Queries the device capabilities.
    {
        if (EINVAL == errno) {                                                  // Error value prints message and quits.
            fprintf(stderr, "%s is no V4L2 device\n",
                     dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
                errno_exit("VIDIOC_QUERYCAP");                                  // Error value prints message and quits.
        }
    }
    printf("cap.capabilities:%x\n",cap.capabilities);                           // Displays the capabilities of the device.
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))                           //Checks if vido capture is a capability.
    {
        fprintf(stderr, "%s is no video capture device\n",
                 dev_name);
        exit(EXIT_FAILURE);
    }
/*******************************************************************************
@brief: Based on type of input i.e. Read-Write and Streaming capability. Switch
*       case is selected.
*******************************************************************************/
    switch (io)
    {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
            {
                fprintf(stderr, "%s does not support read i/o\n",               // Error value prints message and quits. If error value is !=0.
                         dev_name);
                exit(EXIT_FAILURE);
            }
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))                       // Error value prints message and quits. If error value is !=0.
            {
                fprintf(stderr, "%s does not support streaming i/o\n",
                         dev_name);
                exit(EXIT_FAILURE);
            }
            break;
    }


    /* Select video input, video standard and tune here. */


    CLEAR(cropcap);                                                             // memsets the structure with 0s.

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))                              // Queries the device cropping capabilities.
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */                        // Sets the the crop.c field to defrect i.e. default values.

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))                             // Sets the size and location of the sensor window.
        {
            switch (errno)                                                      // Error values are ignored.
            {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                        break;
            }
        }

    }
    else
    {
        /* Errors ignored. */
    }
    
 

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (force_format)
    {
        printf("FORCING FORMAT\n");
        fmt.fmt.pix.width       = HRES;                                         // Sets the HRES to 320
        fmt.fmt.pix.height      = VRES;                                         // Sets the VRES to 240

        // Specify the Pixel Coding Formate here

        // This one work for Logitech C200
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; 
        /*CHECK FOR V4L2_PIX_FMT_UYVY ('UYVY') — Variation of V4L2_PIX_FMT_YUYV with different order of samples in memory
V4L2_PIX_FMT_YVYU ('YVYU') — Variation of V4L2_PIX_FMT_YUYV with different order of samples in memory
V4L2_PIX_FMT_VYUY ('VYUY') — Variation of V4L2_PIX_FMT_YUYV with different order of samples in memory*/

        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY;

        // Would be nice if camera supported
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

        //fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
/*****************************************************************************
Link: http://www.hep.by/gnu/kernel/media/pixfmt.html
V4L2_PIX_FMT_GREY ('GREY') — Grey-scale image
V4L2_PIX_FMT_Y10 ('Y10 ') — Grey-scale image
V4L2_PIX_FMT_Y12 ('Y12 ') — Grey-scale image
V4L2_PIX_FMT_Y10BPACK ('Y10B') — Grey-scale image as a bit-packed array
V4L2_PIX_FMT_Y16 ('Y16 ') — Grey-scale image
*****************************************************************************/
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))                               //VIDIOC_S_FMT : Sets the image format set in fmt.Currently it is FMT_YUV.
                errno_exit("VIDIOC_S_FMT");                                     //http://v4l.videotechnology.com/dwg/v4l2api/v4l2dev.htm

        /* Note VIDIOC_S_FMT may change width and height. */
    }
    else
    {
        printf("ASSUMING FORMAT\n");
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))                               //VIDIOC_G_FMT : Gets the image format set in fmt.Currently it is FMT_YUV.
                    errno_exit("VIDIOC_G_FMT");
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;                                     //Bytesperline is the number of bytes of memory between two adjacent lines.
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;                        //Sets size of frame.
    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;

    switch (io)
    {
        case IO_METHOD_READ:
            init_read(fmt.fmt.pix.sizeimage);                                   // Calls the init_read function.
            break;

        case IO_METHOD_MMAP:                                                    // Calls the init_mmap function.
            init_mmap();
            break;

        case IO_METHOD_USERPTR:
            init_userp(fmt.fmt.pix.sizeimage);                                  // Calls the init_userp function with pix.sizeimage as a parameter.
            break;
    }
}


/*******************************************************************************
@brief: Closes the device.
*******************************************************************************/
static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");                                            // Error values are ignored.

        fd = -1;
}

static void open_device(void)
{
        struct stat st;
/*****************************************************************************
@brief: "stat" : Information about file pointed to by dev_name.This info is 
*       returned in file pointed to by st.
*****************************************************************************/
        if (-1 == stat(dev_name, &st)) {    
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
/*****************************************************************************
@Link: https://octave.sourceforge.io/octave/function/S_ISCHR.html
@brief: S_ISCHR returns the true if the device exists.
*****************************************************************************/
        if (!S_ISCHR(st.st_mode)) { //If '0' indicates no device.               
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }
/*****************************************************************************
@Link: http://man7.org/linux/man-pages/man2/open.2.html
@brief: 'open' opens 'dev_name', in Read-Write and Non-blocking mode.
@param: 1. dev_name - Pointer to device.
*       2. (O_RDWR | O_NONBLOCK) - Read-Write and Non Block flags.
*       3. 0 - Ignored since O_CREAT or O_TMPFILE are not used.
*****************************************************************************/
        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}
/*******************************************************************************
*@brief: Function prints out the options for user. The deatils are printed below.
*@param: fp - File pointer.
*        argc - Takes the number of arguments.
*        argv - The strings which are entered by users.
*******************************************************************************/
static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-f | --format        Force format to 640x480 GREY\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruofc:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};


int main(int argc, char **argv)
{
/*****************************************************************************
* Default value of the argc is considered to be 0. dev_name will be set to 
* "/dev/video0".
*****************************************************************************/
    if(argc > 1)                                                              
        dev_name = argv[1];                                                   
    else
        dev_name = "/dev/video0";

    for (;;)
    {
        int idx;
        int c;

/*******************************************************************************
* @brief: getopt_long() gets the command line arguments.
* @params: 1. argc - Command line argument count.
*          2. argv - Command line argument string.
*          3. short_options ==> string with all possible options. 
*             ":" means n argument is needed.
*          4. long_options - pointer to structure stored in getopt.h
*******************************************************************************/
        c = getopt_long(argc, argv,
                    short_options, long_options, &idx); 

        if (-1 == c)
            break;
/*****************************************************************************
 * Sets the suitble mode, which includes, "device", "help", "mmap", "read", 
 * "usrptr", "output" , "format", "count".
 ****************************************************************************/
        switch (c)
        {
            case 0: /* getopt_long() flag */
                break;

            case 'd':
                dev_name = optarg;  //Gets optional cmd argumemts.
                break;

            case 'h':
                usage(stdout, argc, argv);
                exit(EXIT_SUCCESS);

            case 'm':
                io = IO_METHOD_MMAP;                                            //Sets the io method to IO_METHOD_MMAP
                break;

            case 'r':
                io = IO_METHOD_READ;                                            //Sets the io method to IO_METHOD_READ
                break;

            case 'u':
                io = IO_METHOD_USERPTR;                                         //Sets the io method to IO_METHOD_USERPTR
                break;

            case 'o':
                out_buf++;
                break;

            case 'f':                                                           //Default setting which sets the format to record V4L2_PIX_FMT_YUYV
                force_format++;
                break;

            case 'c':
                errno = 0;
/*******************************************************************************
@Links - 1. https:www.tutorialspoint.com/c_standard_library/c_function_strtol.htm
         2. http://pubs.opengroup.org/onlinepubs/009695399/functions/strtol.html
@brief : Converts the initial value of argument in strtol() to a string.
@params: 1. optarg is the user entered string for device address.
         2. base = 0, indicates a decimal number in optarg.
*****************************************************************/
                frame_count = strtol(optarg, NULL, 0);                        
                if (errno)
                        errno_exit(optarg);
                break;

            default:
                usage(stderr, argc, argv);
                exit(EXIT_FAILURE);
        }
    }

    open_device();
    init_device();
    start_capturing();
    mainloop();
    stop_capturing();
    uninit_device();
    close_device();
    fprintf(stderr, "\n");
    return 0;
}
