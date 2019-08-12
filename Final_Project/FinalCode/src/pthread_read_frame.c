#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_read_frame.h"

void* ReadFrame(void)
{
	struct v4l2_buffer buf;
	unsigned int i;

	fd_set fds;                                                                             // Predefined buffer. 

	struct timeval tv;
	int r;
	int EINTR_flag = 0;
	
	while(PROCESS_COMPLETE_BIT!=1)
	{

	     	   sem_wait(&semA);		 
	    	   clock_gettime(CLOCK_REALTIME, &start_read_frame);
		   exec_time.read_thread[OldframeCount][START]=GetTime(&start_read_frame);

	    	    CLEAR(buf);
	    	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Type is set as enum value = 1.
	    	    buf.memory = V4L2_MEMORY_MMAP; 

	    		    FD_ZERO(&fds);                                                                          // Clears the fds structure.
	    		    FD_SET(fd, &fds);

	    		    /* Timeout. */
	    		    tv.tv_sec = 5;                                                                          // Maximum timeout limited up-to 2s.
	    		    tv.tv_usec = 0;

	    		    r = select(fd + 1, &fds, NULL, NULL, &tv);                                              // Waits for up to 2s before capturing an image.

	    		    if (-1 == r)                                                                            // Indicates failure
	    		    {
	    		        if (EINTR == errno)
	    		            EINTR_flag = 1;
	    		        else
	    		        {
	    		            syslog(LOG_ERR,"[TIME:%fus]select timeout.",TimeValues());                      // Prints the error message.
	    		            
	    		        }
	    		        
	    		    }

	    		    if (0 == r)                                                                             // Indicates timeout.
	    		    {
	    		        syslog(LOG_ERR,"[TIME:%fus]select timeout.",TimeValues());                          // Prints the error message.
	    		        
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
						    syslog(LOG_ERR,"[TIME:%fus]VIDIOC_DQBUF",TimeValues());                     // Error value prints message and quits.
						
					}
	    			
	    		    }

	    		    common_struct.ptr  = buffers[buf.index].start;
	    		    common_struct.SIZE = buf.bytesused;

	    		    assert(buf.index < n_buffers);

	    		    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	    			syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QBUF",TimeValues());  

	    		    // record when process was called
	    		                                                        // Gets 
	    		    ReadFrameCount+=1;   
			
	    		}
			
			clock_gettime(CLOCK_REALTIME, &stop_read_frame);

			syslog(LOG_INFO,"[TIME:%0.3fs]!![Stop  time pthread_read]",TimeValues());
			syslog(LOG_INFO,"[TIME:%0.3fs]!![Diff  time pthread_read]:%0.3fs",TimeValues(),  ExecutionTimeCal(&start_read_frame,&stop_read_frame));
			 exec_time.read_time[independent]= ExecutionTimeCal(&start_read_frame,&stop_read_frame);

    			TASK_BITMASK |= READ_BITMASK;
		    
    		}

}

