#include "SYSLOG.h"
#include "camera.h"

int xioctl(int fh, int request, void *arg)
{
        int r;

        do 
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
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
	syslog(LOG_ERR,"[TIME:%fus]VIDIOC_STREAMOFF.",TimeValues());                                // Prints the error message.
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

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))                                                    //loads the queue(enqueue) for incoming data.
		{
		    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QBUF.", TimeValues());                                // Prints the error message.
		    return -1;
        }
	}

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                             // Type is loaded with Video Capture enum.
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))                                                   // Starts the capture process.
    {
	    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_STREAMON", TimeValues());                             // Prints the error message.
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
  circ_write = 0;
  circ_read = 99;

  Dumpreq.tv_nsec = _20ms_;
  Dumpreq.tv_sec = 0;
  //Readreq.tv_nsec = _30ms_;    	
  //Readreq.tv_sec = 0;

  //req.tv_nsec = _10ms_;
  //req.tv_sec = 0;
      
  OldframeCount=1;

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
        fmt.fmt.pix.width       = HRES;                                                          // Sets the HRES to 640
        fmt.fmt.pix.height      = VRES;                                                          // Sets the VRES to 480


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
