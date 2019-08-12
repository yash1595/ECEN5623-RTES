#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_gray.h"

/*****************************************************************
*@brief : Function converts images from ring buffer and converts 
*         it to gray scale
*@params: None
*@return: None
*****************************************************************/
void* pthread_grey(void)
{
  while(PROCESS_COMPLETE_BIT != 1)							//While process bit is not incomplete
  {
      sem_wait(&semGray);						                //Waits on gray semaphore
      
      if(ReadFrameCount < SKIP_FRAMES)
      {
	continue;
      } 
      syslog(LOG_INFO,"START CALCULATIONS ");
      clock_gettime(CLOCK_REALTIME, &start_pthread_grey);
      YUYV2GREY((unsigned char*)common_struct.ptr, common_struct.SIZE);					//Converts YUYV image to gray
      syslog(LOG_INFO,"[TIME:%0.3fs]FRAME WRITTEN:%d",TimeValues(), TotalFrameCount);	
      CIRCULAR_BUFFER[circ_write].ptr = &gray_ptr[0];					//Stores image pointer on ring buffer
      CIRCULAR_BUFFER[circ_write].SIZE = common_struct.SIZE/2;				//Stores size of image
      CIRCULAR_BUFFER[circ_write].conv_type = 'g';					//Stores the conversion type
      CIRCULAR_BUFFER[circ_write].frame_count = TotalFrameCount;				//Stores frame count
      CIRCULAR_BUFFER[circ_write].frame_conv_time = &frame_time;			//Stores the time of frame capture
      //TotalFrameCount+=1;									//Increments the frame count
      clock_gettime(CLOCK_REALTIME, &stop_pthread_grey);				
      
      syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Stop  time pthread_grey]",TimeValues());		//Gets stop time of gray thread
      syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Diff  time pthread_grey]%0.3fs",TimeValues(), ExecutionTimeCal(&start_pthread_grey,&stop_pthread_grey));	//Gets execution time of gray thread
      exec_time.grey_time[independent] = ExecutionTimeCal(&start_pthread_grey,&stop_pthread_grey); //Stores execution time in global structure  

      circ_write = (circ_write+1)%100;							//Increments write pointer of ring buffer

      TASK_BITMASK |= GREY_BITMASK;							//Sets the gray bit-mask

    }
    pthread_exit(0);
    //return 0;
}

/*******************************************************************************
*@brief  : Converts the YUYV values in pointer by *pptr to Grey scale values
*@params : pptr - The pointer value of unsigned char p
*          size - The number of pixels captured by camera
*@return:  On success returns 0
*******************************************************************************/
int YUYV2GREY(unsigned char* pptr,int size)
{
	clock_gettime(CLOCK_REALTIME, &start_f);
	clock_gettime(CLOCK_REALTIME, &frame_time);
	static int i=0,newi=0;
	for(i=0,newi=0; i<size; newi+=2,i+=4)                                                           
	{
		gray_ptr[newi]=pptr[i];
		gray_ptr[newi+1]=pptr[i+2];
	}

	clock_gettime(CLOCK_REALTIME, &stop);

	return 0;
}
