#include "SYSLOG.h"
#include "deadline.h"
#include "nonblur.h"
#include "main.h"

/*******************************************************************************
*@brief  : Captures frames for comparison
*@params : None
*@return : None
*******************************************************************************/
int FrameCheck(void)
{

	    	    struct v4l2_buffer buf;

	    	    fd_set fds;                                                                             // Predefined buffer. 
	    	    struct timeval tv;
	    	    int r;
	    	    int EINTR_flag = 0;

	    	    CLEAR(buf);
	    	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                             // Type is set as enum value = 1.
	    	    buf.memory = V4L2_MEMORY_MMAP; 

	    		    FD_ZERO(&fds);                                                                          // Clears the fds structure.
	    		    FD_SET(fd, &fds);

	    		    /* Timeout. */
	    		    tv.tv_sec = 5;                                                                          // Maximum timeout limited up-to 5s.
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
			    clock_gettime(CLOCK_REALTIME, &frame_time);				
	    		    COMM_ARRAY[common_struct_index].ptr  = (unsigned char*)buffers[buf.index].start;	//Stores image pointer
	    		    COMM_ARRAY[common_struct_index].SIZE = buf.bytesused;		//Stores size of image size
			    COMM_ARRAY[common_struct_index].frame_conv_time = &frame_time;	//stores the captured frame time
			    COMM_ARRAY[common_struct_index].frame_count = ReadFrameCount;	//Stores the read frame count

	    		    assert(buf.index < n_buffers);

	    		    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	    			syslog(LOG_ERR,"[TIME:%fus]VIDIOC_QBUF",TimeValues());  
  
	    		    ReadFrameCount+=1;   						//Increments the frame count
		   		    
		syslog(LOG_INFO,"[TIME:%0.3fs]New Frame Count : %d", TimeValues(),COMM_ARRAY[common_struct_index].frame_count);
	    		}
		return 0;

}

/*****************************************************************
*@brief : Finds the shifted frame and sets a 500ms from that time
*@params: None
*@return: 0 on success
*****************************************************************/
int DisplayFrameDiff(void)
{
    static int j=0,err_count=0;
    char first_frame[(IMG_SIZE)], compared_frame[IMG_SIZE];

    for(common_struct_index=1; common_struct_index < TEST_FRAMES; common_struct_index+=1)//Compares the frames for blur images
    {        
	err_count=0;
	memcpy(&first_frame[0],STORE_GRAY[common_struct_index-1],IMG_SIZE);  //Copies gray images int local array for comparison
	memcpy(&compared_frame[0],STORE_GRAY[common_struct_index],IMG_SIZE); //Copies gray images int local array for comparison
        syslog(LOG_INFO,"\n[TIME%0.3f]>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>SIZE:%d>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n",TimeValues(), COMM_ARRAY[common_struct_index-1].SIZE);
	for(j=0; j<IMG_SIZE; j+=1)					     //Compares each frame for blur threshold values
	{
	
	if((first_frame[j] - compared_frame[j]) < lower_limit_thresh || (first_frame[j] - compared_frame[j]) > upper_limit_thresh) 
		{
		  err_count++;	
		}
	}
	syslog(LOG_INFO,"[TIME:%0.3f]ERR_COUNT[%d]", TimeValues(), err_count );	//Displays the error count for a frame
	if(err_count > shifted_frame_thresh)
	{
		syslog(LOG_INFO,"[TIME:%0.3f]Array[%d]", TimeValues(), common_struct_index+1 );	//Displays array index with blur image
		clock_gettime(CLOCK_REALTIME, &stop_time);
				
		syslog(LOG_INFO,"\n[TIME:%f]-------------------------------------Stop Time:%fs", TimeValues(),(stop_time.tv_sec+stop_time.tv_nsec*DecNano));
		struct timespec frame_time_delay;
		clock_gettime(CLOCK_REALTIME, &frame_time_delay);
		//Gets the time difference between blur frame capture and current time
		diff_sec = frame_time_delay.tv_sec - COMM_ARRAY[common_struct_index].frame_conv_time->tv_sec;
		diff_nsec = frame_time_delay.tv_nsec - COMM_ARRAY[common_struct_index].frame_conv_time->tv_nsec;

		if(diff_nsec < 0){
		  diff_nsec = COMM_ARRAY[common_struct_index].frame_conv_time->tv_nsec - frame_time_delay.tv_nsec;
		diff_sec+=1;
		}
		//Sets a delay of 500ms from the exact time the blur frame was detected
		syslog(LOG_INFO,"[TIME:%0.3fs]Stop - Start: %d", TimeValues(), _500ms_ - diff_nsec);

		frame_time_delay.tv_nsec += (_500ms_ - diff_nsec);

		if(frame_time_delay.tv_nsec >= nano){
		  frame_time_delay.tv_nsec -= nano;
		frame_time_delay.tv_sec+=1;
		}

		clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&frame_time_delay,NULL); //Starts the delay
		return 0;
	}
   
    }
    return 0;
}


/*****************************************************************
*@brief : Stores the images in local storage for later comparison
*@params: None
*@return: None
*****************************************************************/
void StoreFrameCheck(void)
{
        char check_dumpname[]="img/testG00000000.pgm";
	char check_pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
	static int check_index = 0, dumpfd;
	char check_ptr[IMG_SIZE];
	int written = 0, total;
	static int i=0,newi=0;
	unsigned char* pptr;
	
	written = 0;
	total = 0;
	dumpfd = 0;
		
	pptr = (unsigned char*)COMM_ARRAY[common_struct_index].ptr;

   	for(i=0,newi=0; i<COMM_ARRAY[common_struct_index].SIZE; newi+=2,i+=4)                                                           
	    {
		check_ptr[newi]=pptr[i];
		check_ptr[newi+1]=pptr[i+2];
	    }
		memcpy(&STORE_GRAY[check_index], &check_ptr[0], IMG_SIZE);
		check_index+=1;
		snprintf(&check_dumpname[8], 9, "%08d", COMM_ARRAY[common_struct_index].frame_count+1);    //Populates the header name
		strncat(&check_dumpname[16], ".pgm", 5);						   //Populates the header name
		dumpfd = open(check_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);		//Opens the file with header name

		snprintf(&check_pgm_header[4], 11, "%010d", (int)COMM_ARRAY[common_struct_index].frame_conv_time->tv_sec); //Populates the header name
		strncat(&check_pgm_header[14], " sec ", 5); //Populates the header name
		snprintf(&check_pgm_header[19], 11, "%010d", (int)(COMM_ARRAY[common_struct_index].frame_conv_time->tv_nsec)/1000000);
		//Populates the header name
		strncat(&check_pgm_header[29], " msec \n"H640" "V480"\n255\n", 19); //Populates the header name
	 
		written=write(dumpfd, check_pgm_header, sizeof(check_pgm_header));  //Writes header name in file
		total=0;

		do
		{
		  written=write(dumpfd, check_ptr, COMM_ARRAY[common_struct_index].SIZE/2); //Writes image into local storage
		  total+=written;
		} while(total < COMM_ARRAY[common_struct_index].SIZE/2);
		printf("check_index:%d\n",common_struct_index);
	    	close(dumpfd);
}

/*****************************************************************
*@brief : Provides delay of 100ms between capture of each frame
*@params: None
*@return: None
*****************************************************************/
void FrameDelay(void)
{
	static struct timespec framecheck;
	clock_gettime(CLOCK_REALTIME, &framecheck);
	framecheck.tv_nsec += _100ms_;
	if(framecheck.tv_nsec >= nano)
	{
	  framecheck.tv_nsec -= nano;
	  framecheck.tv_sec+=1;
	}
	clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&framecheck,NULL);
}
