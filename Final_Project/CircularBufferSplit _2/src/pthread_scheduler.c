#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_scheduler.h"

void* Scheduler(void)
{
  static int TotalFrameCount = TotalFrames;
  static int sleep_timer = 0;
  static int StopTime=0;
  
  struct timespec req, rem;
  clock_gettime(CLOCK_REALTIME, &req);

  while(TotalFrameCount > 0)
  {
	    req.tv_nsec += _10ms_;
	    if(req.tv_nsec >= 1000000000){
		  req.tv_nsec -= 1000000000;
                req.tv_sec+=1;
	    }
	    clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&req,NULL);
	    syslog(LOG_INFO,"[TIME:%0.3f]!!@[Scheduler Start]", TimeValues());
	    sleep_timer = (sleep_timer+1);
if(CAPTURE_RATE == 10) 
{  
      	   if((sleep_timer & 1) == 0)
	    {
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![Start time pthread_read]", TimeValues());
		    sem_post(&semA);
	    }

  	    if(sleep_timer == 10)
	    {
		    sleep_timer = 0;
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![Start time pthread_grey]", TimeValues());		    
		    sem_post(&semB);
	    }
}
else
{
	    if(sleep_timer == 100 )
	    {
		    sleep_timer = 0;
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![Start time pthread_read]", TimeValues());
		    sem_post(&semA);
		    syslog(LOG_INFO,"[TIME:%0.3fs]!![Start time pthread_grey]", TimeValues());
		    sem_post(&semB);
	    }
}
	    if(TASK_BITMASK == (SCHED_BITMASK))
	    {
		//syslog(LOG_INFO,"[TIME:%f]!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", TimeValues());
		TASK_BITMASK = 0;	
		TotalFrameCount-=1;
		syslog(LOG_INFO,"[TIME:%0.3f]TotalFrameCount>>>>>>>>>>>>>>>>>>>>>>>>>%d\n", TimeValues(), OldframeCount);
	    }
       
  }
  PROCESS_COMPLETE_BIT = 1;
 
  sem_post(&semA);
  sem_post(&semB);
 
}
