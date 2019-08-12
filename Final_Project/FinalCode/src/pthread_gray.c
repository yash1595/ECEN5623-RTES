#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_gray.h"

/*******************************************************************************
@brief: Takes 100 frames in a continuous loop. 
*******************************************************************************/
void* pthread_grey(void)
{
  while(PROCESS_COMPLETE_BIT != 1)
  {
      
      syslog(LOG_INFO,"[TIME:%f]Grey thread",TimeValues());
      sem_wait(&semB);
      
      if(ReadFrameCount < 18){continue;}
      clock_gettime(CLOCK_REALTIME, &start_pthread_grey);
      
      YUYV2GREY(common_struct.ptr, common_struct.SIZE);

      CIRCULAR_BUFFER[circ_write].ptr = &gray_ptr[0];
      CIRCULAR_BUFFER[circ_write].SIZE = common_struct.SIZE/2;
      CIRCULAR_BUFFER[circ_write].conv_type = 'g';
      CIRCULAR_BUFFER[circ_write].frame_count = OldframeCount;
      CIRCULAR_BUFFER[circ_write].frame_conv_time = &frame_time;
      OldframeCount+=1;
      clock_gettime(CLOCK_REALTIME, &stop_pthread_grey);
      
      syslog(LOG_INFO,"[TIME:%0.3fs]Written check %d",TimeValues(), OldframeCount-1);
      syslog(LOG_INFO,"[TIME:%0.3fs]!![Stop  time pthread_grey]",TimeValues());
      syslog(LOG_INFO,"[TIME:%0.3fs]!![Diff  time pthread_grey]%0.3fs",TimeValues(),  ExecutionTimeCal(&start_pthread_grey,&stop_pthread_grey));
      exec_time.grey_time[independent]= ExecutionTimeCal(&start_pthread_grey,&stop_pthread_grey);      

	

      //syslog(LOG_INFO,"[TIME:%0.3fs]-----------------------------Frame Count:%d.",TimeValues(),CIRCULAR_BUFFER[circ_write].frame_count);
      circ_write = (circ_write+1)%100;

      TASK_BITMASK |= GREY_BITMASK;

    }

    
}


/*******************************************************************************
@brief  : Converts the YUYV values in pointer by *pptr to Grey scale values.
@params : pptr - The pointer value of unsigned char p.
*         size - The number of pixels captured by camera.
*******************************************************************************/
int YUYV2GREY(unsigned char* pptr,int size)
{
  clock_gettime(CLOCK_REALTIME, &start_f);
  clock_gettime(CLOCK_REALTIME, &frame_time);
  static int i=0,newi=0;
//syslog(LOG_INFO,"[TIME:%f]!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", TimeValues());
    for(i=0,newi=0; i<size; newi+=2,i+=4)                                                           // YY is used to store the gray scale values from the YUYV.
    {
        gray_ptr[newi]=pptr[i];
        gray_ptr[newi+1]=pptr[i+2];
    }

    clock_gettime(CLOCK_REALTIME, &stop);
    
    return 0;
}
