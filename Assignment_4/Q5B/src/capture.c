#include "main.h"

extern  enum io_method   io;
extern int              fd;
extern struct buffer          *buffers;
extern  unsigned int     n_buffers;
extern  int              out_buf;
extern  int              force_format;
extern  int              frame_count;
extern unsigned char gray_ptr[];

extern char pgm_header[];
extern char pgm_dumpname[];
extern char pgm_dumpname_grey[];
extern char pgm_dumpname_bright[];
extern char pgm_dumpname_contrast[];
extern char ppm_header[];
extern char ppm_dumpname[];
extern unsigned int framecnt;

extern int HRES[];
extern int VRES[];
extern char* dev_name;
extern unsigned int pgm_header_len;
extern unsigned int ppm_header_len;
char* resolution_string[]={"160x120","320x240","640x480","800x600","960x720"};
char* conversion_string[]={"GRAY","BRIGHTNESS","CONTRAST"};
char* deadline_status[]={"Within ","Missed "};
unsigned char bigbuffer[(1280*960)];


static int ReadFrameCount = 0;

float Start_Finish[NumberOfConversions][TotalFrames];

/**********************************************************
@brief: Function takes the current clock values and calculates
*       difference wrt the starting time.
@return: It returns the float values of time difference in us.
**********************************************************/
float TimeValues(void)
{
    clock_gettime(CLOCK_REALTIME, &finish);
    difference.tv_sec  = finish.tv_sec - start.tv_sec;
    difference.tv_nsec = finish.tv_nsec - start.tv_nsec; 
    if (start.tv_nsec > finish.tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec*1000000 + difference.tv_nsec*0.001);
}

/**********************************************************
@brief: Takes difference between specified start and stop 
*       values.
@return: It returns the float values of time difference in ms.
**********************************************************/
float FrameRateCal(void)
{
    frame_difference.tv_sec  = stop.tv_sec - start_f.tv_sec;
    frame_difference.tv_nsec = stop.tv_nsec - start_f.tv_nsec; 
    if (start_f.tv_nsec > stop.tv_nsec) { 
                --frame_difference.tv_sec; 
                frame_difference.tv_nsec += nano; 
            } 
    return (frame_difference.tv_sec*1000 + (frame_difference.tv_nsec*0.000001)); //ms
}

/**********************************************************
@brief: Function takes the current clock values and calculates
*       difference wrt the starting time.
@return: It returns teh float values of time difference in us.
**********************************************************/
float DeadlineCal(void)
{
    dead_difference.tv_sec  = stop_d.tv_sec - start_d.tv_sec;
    dead_difference.tv_nsec = stop_d.tv_nsec - start_d.tv_nsec; 
    if (start_d.tv_nsec > stop_d.tv_nsec) { 
                --frame_difference.tv_sec; 
                frame_difference.tv_nsec += nano; 
            } 
    return (dead_difference.tv_sec*1000 + dead_difference.tv_nsec*0.000001);
}

/**********************************************************
@brief: Function takes the current clock values and calculates
*       difference wrt the starting time.
@return: It returns teh float values of time difference in us.
**********************************************************/
void DisplayDeadlines(void)
{
 	static int i=0;
        syslog(LOG_INFO,"[TIME:%F]**********************[%s]**********************", TimeValues(),resolution_string[resolution]);
	for(i=0;i<NumberOfConversions;i+=1)
	{
		syslog(LOG_INFO,"[TIME:%F]==================[%s]==================", TimeValues(),conversion_string[i]);
		syslog(LOG_INFO,"[TIME:%F]WCET:%fms", TimeValues(),wcet(i));
		syslog(LOG_INFO,"[TIME:%F]Best Case :%fms", TimeValues(),bcet(i));
		syslog(LOG_INFO,"[TIME:%F]Average:%fms", TimeValues(),average(i));
		syslog(LOG_INFO,"[TIME:%F]==========================================", TimeValues());

	}

}

/**************************************************************************************************
@brief: All the deadlines of each frame for each tranform are displayed.
*       Also displays the start times of each conversion.
***************************************************************************************************/
void DisplayAllDeadlines(void)
{
   int i=0,j=0;
   printf("Time taken by each frame\n");
   for(i=0;i<3;i+=1)
   {
      for(j=0;j<TotalFrames;j+=1)
      {
        printf("[%s]--[%s]--[%f]\n",resolution_string[resolution],conversion_string[i],deadline_storage[i][j]);
      }
        
   }
   printf("Time after each frame\n");
   for(i=0;i<3;i+=1)
   {
      for(j=0;j<TotalFrames;j+=1)
      {
        printf("[%s]--[%s]--[%f]\n",resolution_string[resolution],conversion_string[i],Start_Finish[i][j]);
      }
        
   }
   
}

/***************************************************************************************************
@brief: Displays the worst case execution time.
***************************************************************************************************/
float wcet(int conv_type)
{
   static int i=0;
   float max = 0;
   
   for(i=0;i<TotalFrames;i+=1)
   {
	if(max < deadline_storage[conv_type][i])
		max = deadline_storage[conv_type][i];
	
   }
   
  return max;
}

/***************************************************************************************************
@brief: Displays the best case execution time.
***************************************************************************************************/
float bcet(int conv_type)
{
   static int i=0;
   float min = 30000.0;
   for(i=0;i<TotalFrames;i+=1)
   {
	 if(min > deadline_storage[conv_type][i] && deadline_storage[conv_type][i]!=0.0)
		 min = deadline_storage[conv_type][i];
   }
   
  return min;
}

/***************************************************************************************************
@brief: Displays the average execution time.
***************************************************************************************************/
float average(int conv_type)
{
   static int i=0;
   float sum = 0;
   
   for(i=0;i<TotalFrames;i+=1)
   {
	   sum+=deadline_storage[conv_type][i];
   }
  
  return (sum/TotalFrames);
}

/*********************************************************************
@Link: // https://unix.superglobalmegacorp.com/Net2/newsrc/sys/syslog.h.html
@brief: Initializes the syslog with "[Capture]" event.
*       Error values are stored in LOG_LOCAL0.
*       Values upto LOG_DEBUG can be logged.
**********************************************************************/
void SyslogInit(void)
{
    printf("All printf statements will be replaced by syslogs.\n");
    printf("DEADLINE is taken as 2s for %d frames.\n",TotalFrames);
    openlog("[960x720]",LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    setlogmask(LOG_UPTO(LOG_DEBUG)); 
    syslog(LOG_INFO,"<<<<<<<<<<<<<<<<<<<<<<<BEGIN>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
}

/*****************************************************************************
*@Link: http://man7.org/linux/man-pages/man2/ioctl.2.html.
==============================================================================
*@brief: Recursively runs 'ioctal' till error value is not returned.
*@param: 1. fh - File pointer.
*        2. request - 
*        3. arg - struct v4l2_buffer*.
*@return: Value which is interpreted by the calling function.
*******************************************************************************/
int xioctl(int fh, int request, void *arg)
{
        int r;

        do 
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
}

/*****************************************************************************
@brief: Saves the image in pgm format.
@params: p    -> Pointer to the conversion string.
*        x    -> Character for grey, brightness or contrast.
*        size -> The size to write to the pgm_header.
*        tag  -> The ID for the specific frame.
*        time -> Stores the time in sec and ms.
*****************************************************************************/
void dump_pgm(const void *p, char x,int size, uint32_t tag, struct timespec *time)
{
    
    int written,total, dumpfd;
    static uint32_t bright_count = TotalFrames+1;
    static uint32_t cont_count = (2*TotalFrames)+1;

    switch(x)                                                                                       // Selects the suitable pgm_dumpname string.
    {
    	case 'g':
    		snprintf(&pgm_dumpname_grey[4], 9, "%08d", tag);
        		strncat(&pgm_dumpname_grey[12], ".pgm", 5);
        		dumpfd = open(pgm_dumpname_grey, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
     		break;
    	case 'b':
    		 snprintf(&pgm_dumpname_bright[4], 9, "%08d", (tag+bright_count));
        		strncat(&pgm_dumpname_bright[12], ".pgm", 5);
        		dumpfd = open(pgm_dumpname_bright, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
     		break;
    	case 'c':
    		 snprintf(&pgm_dumpname_contrast[4], 9, "%08d", (tag+cont_count));
        		strncat(&pgm_dumpname_contrast[12], ".pgm", 5);
        		dumpfd = open(pgm_dumpname_contrast, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
     		break;


    }
   

    snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&pgm_header[14], " sec ", 5);
    snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));

    switch(resolution)                                                                              // Selects the resoluiton dynamically.
    {
        case 0:
            strncat(&pgm_header[29], " msec \n"H160" "V120"\n255\n",  19);
            break;
        case 1:
            strncat(&pgm_header[29], " msec \n"H320" "V240"\n255\n", 19);
            break;
        case 2:
            strncat(&pgm_header[29], " msec \n"H640" "V480"\n255\n", 19);
            break;
        case 3:
            strncat(&pgm_header[29], " msec \n"H800" "V600"\n255\n", 19);
            break;
        case 4:
            strncat(&pgm_header[29], " msec \n"H960" "V720"\n255\n", 19);
            break;
    }

    written=write(dumpfd, pgm_header, pgm_header_len);
    total=0;

    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    close(dumpfd);
    
}

/*******************************************************************************
@brief: Takes 50 frames in a continuous loop. 
*******************************************************************************/
void* pthread_grey(void)
{

    while(PROCESS_COMPLETE_BIT != 1)
    {
    syslog(LOG_INFO,"[TIME:%f]Grey thread",TimeValues());
    sem_wait(&semA);

    if(PROCESS_COMPLETE_BIT)
      break;
    clock_gettime(CLOCK_REALTIME,&start_d);

    YUYV2GREY(common_struct.ptr, common_struct.SIZE);
                                                                                                    // A zero or negative value quits the loop.
    TASK_BITMASK |= GREY_BITMASK;
                                                                                                    // A zero or negative value quits the loop.
        
    frame_rate = FrameRateCal();

    if((frame_rate) < 100.0)
      deadline_index=0;
    else
      deadline_index=1;

    syslog(LOG_INFO,"[TIME:%fus]Resolution[%s] Conversion[%s]Frame rate[%f]ms %sdeadline.", \
    TimeValues(),resolution_string[resolution], conversion_string[0],\
    (frame_rate),deadline_status[deadline_index]);

    deadline_storage[0][OldframeCount[0]]=(frame_rate);

    clock_gettime(CLOCK_REALTIME, &stop_d);
    
    OldframeCount[0]+=1;
    }
   
}

/*******************************************************************************
@brief: Takes 50 frames in a continuous loop. 
*******************************************************************************/
void* pthread_bright(void)
{
    while(PROCESS_COMPLETE_BIT != 1)
    {
        syslog(LOG_INFO,"[TIME:%f]Bright thread",TimeValues());
        sem_wait(&semB);

        if(PROCESS_COMPLETE_BIT)
        break;
        clock_gettime(CLOCK_REALTIME,&start_d);


        BRIGHT(common_struct.ptr, common_struct.SIZE);
                                                                                                    // A zero or negative value quits the loop.
        TASK_BITMASK |= (BRIGHT_BITMASK);
                                                                                                    // A zero or negative value quits the loop.
                  
        frame_rate = FrameRateCal();

        if((frame_rate) < 100.0)
          deadline_index=0;
        else
          deadline_index=1;

        syslog(LOG_INFO,"[TIME:%fus]Resolution[%s] Conversion[%s]Frame rate[%f]ms %sdeadline.", \
        TimeValues(),resolution_string[resolution], conversion_string[1],\
        (frame_rate),deadline_status[deadline_index]);

        deadline_storage[1][OldframeCount[1]]=(frame_rate);

        clock_gettime(CLOCK_REALTIME, &stop_d);

        OldframeCount[1]+=1;
    }
 
}

/*******************************************************************************
@brief: Takes 50 frames in a continuous loop. 
*******************************************************************************/
void* pthread_contrast(void)
{

    while(PROCESS_COMPLETE_BIT != 1)
    {
        syslog(LOG_INFO,"[TIME:%f]Contrast thread",TimeValues());
        sem_wait(&semC);
        //syslog(LOG_INFO,"[TIME:%f]Contrast thread got sem",TimeValues());
        if(PROCESS_COMPLETE_BIT)
            break;
        clock_gettime(CLOCK_REALTIME,&start_d);

        CONTRAST(common_struct.ptr, common_struct.SIZE);
                                                                                                    // A zero or negative value quits the loop.
        TASK_BITMASK |= (CONT_BITMASK);
                                                                                                    // A zero or negative value quits the loop.
                          
        frame_rate = FrameRateCal();

        if((frame_rate) < 100.0)
          deadline_index=0;
        else
          deadline_index=1;

        syslog(LOG_INFO,"[TIME:%fus]Resolution[%s] Conversion[%s]Frame rate[%f]ms %sdeadline.", \
               TimeValues(),resolution_string[resolution], conversion_string[2],\
               (frame_rate),deadline_status[deadline_index]);

        deadline_storage[2][OldframeCount[2]]=(frame_rate);

        clock_gettime(CLOCK_REALTIME, &stop_d);
        deadline=DeadlineCal();
        OldframeCount[2]+=1;

    }

    
}

/*******************************************************************************
@Link: //https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec/rn01re61.html
@brief: Ends the video capturing process.
*******************************************************************************/
int StopCapturing(void)
{
    enum v4l2_buf_type type;

    if(PROCESS_COMPLETE_BIT)

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                             // Type is set as enum value = 1. 
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))                                                  // Stops the video streaming.
    {
	syslog(LOG_ERR,"[TIME:%fus]VIDIOC_STREAMOFF.",TimeValues());                                      // Prints the error message.
    } 
    return 0;
    
}

/*******************************************************************************
@brief: Begins the video capture by loading the VIDIOC_QBUF queue.
*******************************************************************************/
int StartCapturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;
    
    for (i = 0; i < n_buffers; ++i)                                                                 // Initially value is '0'.
    {
		syslog(LOG_INFO,"[TIME:%fus]allocated buffer %d", TimeValues(),i);
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))                                                        //loads the queue(enqueue) for incoming data.
		{
		    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QBUF.", TimeValues());                                    // Prints the error message.
		    return -1;
        }
	}

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                             // Type is loaded with Video Capture enum.
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))                                                   // Starts the capture process.
    {
	    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_STREAMON", TimeValues());                                   // Prints the error message.
        return -1;
    }
    
    return 0;

}

/*******************************************************************************
@brief: Frees all the buffers malloced by MMAP or User-Pointer.
*******************************************************************************/
int UninitDevice(void)
{
/*******************************************************************************
@Link: //http://v4l.videotechnology.com/dwg/v4l2api/v4l2dev.htm
================================================================================
@brief: he munmap() function shall remove any mappings for those entire pages 
* containing any part of the address space of the process starting at addr and
* continuing for len bytes.
@params: Starting address of the buffers[] and length of the same buffer.
*******************************************************************************/ 
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
    {
        if (-1 == munmap(buffers[i].start, buffers[i].length)) 
        {
            syslog(LOG_ERR,"[TIME:%fus]Munmap.", TimeValues());                                     // Prints the error message.
            return -1;
        }
    }
    
    free(buffers);
    return 0;
}


/*******************************************************************************
@Links : http://v4l.videotechnology.com/dwg/v4l2api/v4l2dev.htm         
@brief: Opted by Memroy Mapped I/O.
*******************************************************************************/
int init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 6;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                     // Sets type to Video capture.
        req.memory = V4L2_MEMORY_MMAP;                                                              // The memory is set as MMAP.

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))                                                 // Mmap buffers are allocated by ioctl first before being mapped by applications address place.
        {
                if (EINVAL == errno) 
                {
                        syslog(LOG_ERR,"[TIME:%fus]Memory mapping not supported.",\
                         TimeValues());                                                             // Prints the error message.     
                        return -1;
                } 
                else 
                {
                        syslog(LOG_ERR,"[TIME:%fus]VIDIOC_REQBUFS.", \
                        TimeValues());                                                              // Prints the error message.
                }
        }

        if (req.count < 2)                                                                          // If counts are less than 2 end program.
        {
                 syslog(LOG_ERR,"[TIME:%fus]Insufficient buffer memory.", \
                 TimeValues());                                                                     // Prints the error message.                     
                 return -1;
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) 
        {   
                syslog(LOG_ERR,"[TIME:%fus]Out of memory.", TimeValues());                          // Prints the error message.
                return -1;
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                      // Sets type to Video capture.
                buf.memory      = V4L2_MEMORY_MMAP;                                                 // The memory is set as MMAP.
                buf.index       = n_buffers;                                                        // buf.index is used to store the current n_buffers value. 

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                {
                        syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QUERYBUF", \
                        TimeValues());                                                              // Prints the error message.
                        return -1;
                }  

                buffers[n_buffers].length = buf.length;                                             // Buf length is stored in each .length component of buffers[] structure array.
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
                              PROT_READ | PROT_WRITE /* required */,                                // Pages may be read or written.
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)                                         // Returns this value when mmap fails. MAP_FAILED = (void*)(-1)
                {
                        syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QUERYBUF", TimeValues());                 //http://v4l.videotechnology.com/dwg/v4l2api/v4l2dev.htm
                        return -1;
                }
        }
 return 0;
}


void print_scheduler(void)                                                      // Displays the scheduling policy.
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is SCHED_FIFO", TimeValues());
           break;
     case SCHED_OTHER:
           syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is SCHED_OTHER",TimeValues()); exit(-1);
       break;
     case SCHED_RR:
           syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is SCHED_RR",TimeValues()); exit(-1);
           break;
     default:
       syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is UNKNOWN",TimeValues()); exit(-1);
   }

}
/*******************************************************************************
@brief: Camera is checked for video streaming, cropping the image and capture 
* images in a specified format. In this case, YUYV.
*******************************************************************************/
int InitDevice(void)
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
	Readreq.tv_nsec = _20ms_;    	
	Readreq.tv_sec = 0;

  	req.tv_nsec = _100ms_;
  	req.tv_sec = 0;
      
        OldframeCount[0]=1;
	OldframeCount[1]=1;
	OldframeCount[2]=1;
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;
    PROCESS_COMPLETE_BIT = 0;
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))                                                    // Queries the device capabilities.
    {
        if (EINVAL == errno) 
        {
            syslog(LOG_ERR,"[TIME:%fus]V4L2 device absent.", TimeValues());                         // Prints the error message.
            return -1;
        }
        else
        {
                syslog(LOG_ERR,"[TIME:%fus]V4L2 device absent.", TimeValues());                     // Prints the error message.
                return -1;
        }
    }
    syslog(LOG_INFO,"[TIME:%fus]cap.capabilities:%x\n", TimeValues(),cap.capabilities);
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))                                               //Checks if video capture is a capability.
    {
        syslog(LOG_ERR,"[TIME:%fus]Device does not support video capabilities.", TimeValues());     // Prints the error message.
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))                                                   // Error value prints message and quits. If error value is !=0.
    {
        syslog(LOG_ERR,"[TIME:%fus]Device does not support streaming.", TimeValues());              // Prints the error message.
        return -1;
    }
 
    


    /* Select video input, video standard and tune here. */

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
*****************************************************************************/
    CLEAR(cropcap);                                                                                 // memsets the structure with 0s.

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))                                                  // Queries the device cropping capabilities.
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */                                            // Sets the the crop.c field to defrect i.e. default values.

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))                                                 // Sets the size and location of the sensor window.
        {
            if(errno ==  EINVAL)
            {
                syslog(LOG_ERR,"[TIME:%fus]Unable to set cropping.", TimeValues());                 // Prints the error message.
                return -1;
            }
        }

    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (force_format)
    {
        syslog(LOG_INFO,"[TIME:%fus]FORCING FORMAT.", TimeValues());
        fmt.fmt.pix.width       = HRES[resolution];                                                          // Sets the HRES to 320
        fmt.fmt.pix.height      = VRES[resolution];                                                          // Sets the VRES to 240


        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; 
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))                                                   //VIDIOC_S_FMT : Sets the image format set in fmt.Currently it is FMT_YUV.
        {
            syslog(LOG_ERR,"[TIME:%fus]Unable to set cropping.", TimeValues());                     // Prints the error message.
            return -1;
        }       

        /* Note VIDIOC_S_FMT may change width and height. */
    }


    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
    {
            fmt.fmt.pix.bytesperline = min;                                                         //Bytesperline is the number of bytes of memory between two adjacent lines.
    }
    
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;                                            //Sets size of frame.
    
    if (fmt.fmt.pix.sizeimage < min)
    {
            fmt.fmt.pix.sizeimage = min;
    }
    
    init_mmap();

    return 0;

}

/*******************************************************************************
@brief: Closes the device.
*******************************************************************************/
int CloseDevice(void)
{
        if (-1 == close(fd))
        {
            syslog(LOG_ERR,"[TIME:%fus]File closing error.", TimeValues());                         // Prints the error message.
            return -1;
        }

        fd = -1;

        return 0;
}

int OpenDevice(void)
{
/*****************************************************************************
* Default value of the argc is considered to be 0. dev_name will be set to 
* "/dev/video0".
*****************************************************************************/
	    dev_name = "/dev/video0";
        struct stat st;
/*****************************************************************************
@brief: "stat" : Information about file pointed to by dev_name.This info is 
*       returned in file pointed to by st.
*****************************************************************************/
        if (-1 == stat(dev_name, &st)) {    
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                return -1;
        }
/*****************************************************************************
@Link: https://octave.sourceforge.io/octave/function/S_ISCHR.html
@brief: S_ISCHR returns the true if the device exists.
*****************************************************************************/
        if (!S_ISCHR(st.st_mode)) { //If '0' indicates no device.
                fprintf(stderr, "%s is no device\n", dev_name);
                return -1;
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
                return -1;
        }
 return 0;
}


/*******************************************************************************
@brief  : Converts the YUYV values in pointer by *pptr to Grey scale values.
@params : pptr - The pointer value of unsigned char p.
*         size - The number of pixels captured by camera.
*******************************************************************************/
int YUYV2GREY(unsigned char* pptr,int size)
{
  clock_gettime(CLOCK_REALTIME, &start_f);
  static int i=0,newi=0;

    for(i=0,newi=0; i<size; newi+=2,i+=4)                                                           // YY is used to store the gray scale values from the YUYV.
    {
        gray_ptr[newi]=pptr[i];
        gray_ptr[newi+1]=pptr[i+2];
    }
    dump_pgm(gray_ptr, 'g',(size/2), OldframeCount[0], &frame_time);
    clock_gettime(CLOCK_REALTIME, &stop);
    
    Start_Finish[0][OldframeCount[0]]=TimeValues();
    return 0;
}

/*******************************************************************************
@brief  : Converts the YUYV values in pointer by *pptr to bright scale values.
@params : pptr - The pointer value of unsigned char p.
*         size - The number of pixels captured by camera.
*******************************************************************************/
int BRIGHT(unsigned char* pptr,int size)
{
    clock_gettime(CLOCK_REALTIME, &start_f);
    static int i=0,newi=0;

    for(i=0,newi=0; i<size; newi+=2,i+=4)                                                           // YY is used to store the gray scale values from the YUYV.
    {
        gray_ptr[newi]=(pptr[i]*0.5);
        gray_ptr[newi+1]=(pptr[i+2]*0.5);
    }
    dump_pgm(gray_ptr,'b', (size/2), OldframeCount[1], &frame_time);
    clock_gettime(CLOCK_REALTIME, &stop);
    Start_Finish[1][OldframeCount[1]]=TimeValues();
    return 0;
}

/*******************************************************************************
@brief  : Converts the YUYV values in pointer by *pptr to Contrast scale values.
@params : pptr - The pointer value of unsigned char p.
*         size - The number of pixels captured by camera.
*******************************************************************************/
int CONTRAST(unsigned char* pptr,int size)
{
    clock_gettime(CLOCK_REALTIME, &start_f);
    static int i=0,newi=0;

    for(i=0,newi=0; i<size; newi+=2,i+=4)                                                           // YY is used to store the gray scale values from the YUYV.
    {
	if(pptr[i]<=127)
	   gray_ptr[newi] = pptr[i]*0.5;
	else
	  gray_ptr[newi] = pptr[i]*1.2;
	if(pptr[i+2]<=127)
	  gray_ptr[newi+1] = pptr[i+2]*0.5;
        else
	  gray_ptr[newi+1] = pptr[i+2]*1.2;
    }
    dump_pgm(gray_ptr, 'c',(size/2), OldframeCount[2], &frame_time);	
    clock_gettime(CLOCK_REALTIME, &stop);  
    Start_Finish[2][OldframeCount[2]]=TimeValues();
    return 0;
}

/***************************************************************************************************
@brief: Reads a frame and increments a global counter.
***************************************************************************************************/
void* ReadFrame(void)
{
	while(PROCESS_COMPLETE_BIT!=1)
	{
  		while(nanosleep(&Readreq,&Readrem)!=0)
  		{
  			Readreq.tv_nsec = Readrem.tv_nsec;
  		}


	    struct v4l2_buffer buf;
	    unsigned int i;

	    fd_set fds;                                                                                   // Predefined buffer. 
	    struct timeval tv;
	    int r;
	    int EINTR_flag = 0;

	    CLEAR(buf);
	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                       // Type is set as enum value = 1.
	    buf.memory = V4L2_MEMORY_MMAP; 

		    FD_ZERO(&fds);                                                                              // Clears the fds structure.
		    FD_SET(fd, &fds);

		    /* Timeout. */
		    tv.tv_sec = 5;                                                                              // Maximum timeout limited up-to 2s.
		    tv.tv_usec = 0;

		    r = select(fd + 1, &fds, NULL, NULL, &tv);                                                  // Waits for up to 2s before capturing an image.

		    if (-1 == r)                                                                                // Indicates failure
		    {
		        if (EINTR == errno)
		            EINTR_flag = 1;
		        else
		        {
		            syslog(LOG_ERR,"[TIME:%fus]select timeout.",TimeValues());                          // Prints the error message.
		            
		        }
		        
		    }

		    if (0 == r)                                                                                 // Indicates timeout.
		    {
		        syslog(LOG_ERR,"[TIME:%fus]select timeout.",TimeValues());                              // Prints the error message.
		        
		    }

        if(EINTR_flag == 0)
        {
            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))                                               // https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/vidioc-qbuf.html?highlight=vidioc_dqbuf
            {                                                                                       // Enqueues an empty/full buffer onto the drivers queue.
               
            		switch (errno)
            		{
            		    case EAGAIN:
            			    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_DQBUF-EAGAIN",TimeValues());
            			    exit(1);

            		    case EIO:
            			/* Could ignore EIO, see spec. */
            			    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_DQBUF-EIO",TimeValues());
            			/* fall through */
            			    exit(1);

            		    default:
            			    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_DQBUF",TimeValues());                       // Error value prints message and quits.
            			
            		}

            }

            common_struct.ptr  = buffers[buf.index].start;
            common_struct.SIZE = buf.bytesused;

            assert(buf.index < n_buffers);

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            {
                syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QBUF",TimeValues());  
            }

            clock_gettime(CLOCK_REALTIME, &frame_time);                                             // Gets the current time. 

            ReadFrameCount+=1;                                                                      // ReadFrameCount increments.
                                                                                                    // Increments the framecnt everytime a new frame is captured.
            syslog(LOG_INFO,"[TIME:%fus]New Frame Count : %d", TimeValues(),ReadFrameCount);
		     }
	
    }

}

/***************************************************************************************
@brief: This thread schedules the 3 conversion threads in a RM fasion.
***************************************************************************************/
void* Scheduler(void)
{
  static int TotalFrameCount = TotalFrames;
  syslog(LOG_INFO,"[TIME:%f]In scheduler\n", TimeValues());

  while(TotalFrameCount > 0)
  {
    syslog(LOG_INFO,"[TIME:%f]Scheduler thread",TimeValues());
    if(ReadFrameCount>0)                                                                            // Proceeds only when atleast 1 frame has been captured.
    {
    	sem_post(&semA);                                                                              // Semaphores are posted in every loop.
    	sem_post(&semB);
    	sem_post(&semC);
    }

    if(nanosleep(&req,&rem) < 0)                                                                    // Sleeps for 100ms.
    {
	     req.tv_nsec = rem.tv_nsec;
    }
    

    if(TASK_BITMASK == SCHED_BITMASK)                                                               // If TASK_BITMASK matches 0x07, only then total frame count decreases.
    {
        TotalFrameCount-=1;
        TASK_BITMASK = 0;
	      syslog(LOG_INFO,"[TIME:%f]TotalFrameCount>>>>>>>>>>>>>>>>>>>>>>>>>%d\n", TimeValues(), OldframeCount[0]);
    }
    
  }

  PROCESS_COMPLETE_BIT = 1;                                                                         // Bit set when all the frames have been processed.

  sem_post(&semA);                                                                                  // All semaphores are released so that the program can end gracefully.
  sem_post(&semB);
  sem_post(&semC);
 
}

/***************************************************************************************************
@brief: Assigns each thread to a function pointer.
***************************************************************************************************/
int InitThreads(void)
{
    function_pointer[0]=Scheduler;                        //CORE-0                                  // Scheduler is highest priority.
    function_pointer[1]=ReadFrame;                        //CORE-0

    function_pointer[2]=pthread_grey;                     //CORE-3
    function_pointer[3]=pthread_bright;                   //CORE-3
    function_pointer[4]=pthread_contrast;                 //CORE-3  

    return 0;
}








