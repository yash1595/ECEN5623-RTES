#include "SYSLOG.h"
#include "deadline.h"
#include "main.h"

/*****************************************************************
*@brief : Calculates difference between stop and start times
*@params: Timespec struct with start and stop pointers
*@return: Time difference in float seconds
*****************************************************************/
float ExecutionTimeCal(struct timespec* start, struct timespec* stop)
{
   
   difference.tv_sec = stop->tv_sec - start->tv_sec;
   difference.tv_nsec = stop->tv_nsec - start->tv_nsec; 
    if (start->tv_nsec > stop->tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec + difference.tv_nsec*DecNano);
}

/*****************************************************************
*@brief : Displays all the Average, Best case and worst case
*         execution times
*@params: None
*@return: None
*****************************************************************/
void DisplayAllDeadlines(void)
{
  static int i=0;
  printf(" Start and Stop times for all threads \n");
  for(i=0;i<4;i++)
  {
	SortThreadTime(i);
  }
  

   sum = 0.0;
   syslog(LOG_INFO,"Average Read:%fs\n",(Average(READ)));
   sum = 0.0;
   syslog(LOG_INFO,"Average Grey:%0.3fs\n",(Average(GREY)));
   sum = 0.0;
   syslog(LOG_INFO,"Average Dump:%0.3fs\n",(Average(DUMP)));
   syslog(LOG_INFO,"Average Sched:%0.3fs\n", Average_SCHED());

   syslog(LOG_INFO,"BCET Read:%fs\n",BCET(READ));
   syslog(LOG_INFO,"BCET Grey:%0.3fs\n",BCET(GREY));
   syslog(LOG_INFO,"BCET Dump:%0.3fs\n",BCET(DUMP));
   syslog(LOG_INFO,"BCET Sched:%0.3fs\n",  BCET(SCHEDULER));

   syslog(LOG_INFO,"WCET Read:%fs\n",    WCET(READ));
   syslog(LOG_INFO,"WCET Grey:%0.3fs\n", WCET(GREY));
   syslog(LOG_INFO,"WCET Dump:%0.3fs\n", WCET(DUMP));
   syslog(LOG_INFO,"WCET Sched:%0.3fs\n",WCET(SCHEDULER));

   for(i=0 ; i < 4 ; i+=1)
   {
	GetValues(i);
   }
  
   

}


/*****************************************************************
*@brief : Calculates worst case execution time of mentioned thread
*@params: Thread ID
*@return: WCET value of mentioned thread
*****************************************************************/
float WCET(int thread)
{
 float max;
 switch(thread)
 {
	case READ:
		max = exec_time.read_time[TotalFrames-1];
	break;
	
	case GREY:
		max = exec_time.grey_time[TotalFrames-1];
	break;

	case DUMP:
		max = exec_time.dump_time[TotalFrames-1];
	break;

	case SCHEDULER:
		max = sched_time[independent_sched-1];
	break;

 }
	return max;

}

/*****************************************************************
*@brief : Calculates average case execution time of scheduler
*@params: None
*@return: Average value of mentioned thread
*****************************************************************/
float Average_SCHED(void)
{ 
  static int i=0;
  float sum = 0.0;

  for(i=0; i<independent_sched; i+=1)
  {
	   sum += sched_time[i];
  }
  return sum/independent_sched;
 
}

/*****************************************************************
*@brief : Calculates average case execution time of mentioned 
*         thread
*@params: Thread ID
*@return: Average value of mentioned thread
*****************************************************************/
float Average(int thread)
{
  static int i = 0;
 for(i=0 ; i<TotalFrames ; i+=1)
 {
   switch(thread)
   {
	case READ :
	if(exec_time.read_time[i])
	{
	   sum += exec_time.read_time[i];
	}
	break;

	case GREY :
	if(exec_time.grey_time[i])
	   sum += exec_time.grey_time[i];
	break;

	case DUMP :
	   sum += exec_time.dump_time[i];
	break;

   }
    
 }
 return (sum/TotalFrames);
}

/*****************************************************************
*@brief : Calculates best case execution time of mentioned 
*         thread
*@params: Thread ID
*@return: Best value of mentioned thread
*****************************************************************/
float BCET(int thread)
{
float min;
switch(thread)
 {
	case READ:
		min = exec_time.read_time[0];
	break;
	
	case GREY:
		min = exec_time.grey_time[0];
	break;

	case DUMP:
		min = exec_time.dump_time[0];
	break;

	case SCHEDULER:
		min = sched_time[0];
	break;

 }
	return min;
}

void SortThreadTime(int thread)
{
	static int i=0, j=0;
 switch(thread)
 {
 	case GREY:
	 	for(i=0 ; i<TotalFrames ; i+=1)
		{
			for(j=i+1 ; j<TotalFrames ; j+=1)
			{
				if(exec_time.grey_time[j] < exec_time.grey_time[i])
				{
					exec_time.grey_time[i]  = exec_time.grey_time[j];
				}
			}
		}
	break;

	case READ:
	 	for(i=0 ; i<TotalFrames ; i+=1)
		{
			for(j=i+1 ; j<TotalFrames ; j+=1)
			{
				if(exec_time.read_time[j] < exec_time.read_time[i])
				{
					exec_time.read_time[i]  = exec_time.read_time[j];
				}
			}
		}
	break;

	case DUMP:
	 	for(i=0 ; i<TotalFrames ; i+=1)
		{
			for(j=i+1 ; j<TotalFrames ; j+=1)
			{
				if(exec_time.dump_time[j] < exec_time.dump_time[i])
				{
					exec_time.dump_time[i]  = exec_time.dump_time[j];
				}
			}
		}
	break;
	
	case SCHEDULER:
	 	for(i=0 ; i<independent_sched ; i+=1)
		{
			for(j=i+1 ; j<independent_sched ; j+=1)
			{
				if(sched_time[j] < sched_time[i])
				{
					sched_time[i]  = sched_time[j];
				}
			}
		}
	break;
 }

}


void GetValues(int thread)
{
  int i = 0;
  switch(thread)
  {
	case GREY:
		for(i=0;i<TotalFrames;i+=1)
			syslog(LOG_INFO," RMA Grey[%d]:%f\n",i,exec_time.grey_time[i]);
	break;

	case READ:
		for(i=0;i<TotalFrames;i+=1)
			syslog(LOG_INFO," RMA Read[%d]:%f\n",i,exec_time.read_time[i]);
	break;

	case DUMP:
		for(i=0;i<TotalFrames;i+=1)
			syslog(LOG_INFO," RMA Dump[%d]:%f\n",i,exec_time.dump_time[i]);
	break;
	
	case SCHEDULER:
		for(i=0;i<independent_sched;i+=1)
			syslog(LOG_INFO," RMA Scheduler[%d]:%f\n",i,sched_time[i]);
	break;
     
  }

}

