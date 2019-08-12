#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_scheduler.h"

/*****************************************************************
*@brief : Function manages the RM threads. Runs read frame at 50ms
*@params: None
*@return: None
*****************************************************************/
void* Scheduler(void)
{
  TotalFrameCount = 0;
  static int sleep_timer = 0;
  struct timespec req;
  clock_gettime(CLOCK_REALTIME, &req);

  while(TotalFrameCount <= TotalFrames)							                //Runs till total frames are not captured
  {
	    req.tv_nsec += _10ms_;
	    if(req.tv_nsec >= nano){
		  req.tv_nsec -= nano;
                req.tv_sec+=1;
	    }
	    syslog(LOG_INFO,"[TIME:%0.3f]!![JITTER Start time pthread_scheduler]", TimeValues());      //Displays start time of scheduler
	    clock_gettime(CLOCK_REALTIME, &start_scheduler);
	    clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&req,NULL);				//Sets absolute time for 10ms sleep
	    
	    sleep_timer = (sleep_timer+1);
if(CAPTURE_RATE == _10HZ_MODE) 
{  
      	   if((sleep_timer % divide_by_50ms) == 0)						//Triggers after 50ms
	    {		    
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Start time pthread_read]", TimeValues());  //Displays start time of read thread
		    sem_post(&semRead);								//Posts the semaphore for read thread
	    }

  	    if(sleep_timer % divide_by_100ms == 0)						//Triggers after 100ms
	    {
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Start time pthread_grey]", TimeValues());	//Displays start time of gray thread scheduler	    
		    sem_post(&semGray);								//Posts the semaphore for gray thread
	    }
}
else
{
	    if((sleep_timer % divide_by_50ms) == 0)						//Triggers after 50ms
	    {
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Start time pthread_read]", TimeValues());
		    sem_post(&semRead);								//Posts the semaphore for read thread
	    }

	    if(sleep_timer % divide_by_1s == 0 )						//Triggers after 1s
	    {
		    
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Start time pthread_grey]", TimeValues());
		    sem_post(&semGray);								//Posts the semaphore for gray thread
	    }
}
	    if(TASK_BITMASK == SCHED_BITMASK)							//Checks if Read and gray conversion was complete
	    {	
		TASK_BITMASK = 0;	
		TotalFrameCount+=1;
		syslog(LOG_INFO,"[TIME:%0.3f]TotalFrameCount>>>>>>>>>>>>>>>>>>>>>>>>>%d\n", TimeValues(), TotalFrameCount);
		independent+=1;
	    }

	    clock_gettime(CLOCK_REALTIME, &stop_scheduler);

	    syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Stop  time pthread_scheduler]",TimeValues());     //Displays stop time of scheduler
            syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Diff  time pthread_scheduler] %0.3fs",TimeValues(),ExecutionTimeCal(&start_scheduler,&stop_scheduler));		       //Displays execution time of scheduler
	    sched_time[independent_sched]= ExecutionTimeCal(&start_scheduler,&stop_scheduler); //Stores execution time in global structure
	    
	    independent_sched+=1;							       //Increments the independent_sched counter value
       
  }
  PROCESS_COMPLETE_BIT = 1;
 
  sem_post(&semRead);									       //Posts read semaphore
  sem_post(&semGray);									       //Posts gray semaphore
  pthread_exit(0);
}
