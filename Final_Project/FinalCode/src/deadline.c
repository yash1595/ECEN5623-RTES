#include "SYSLOG.h"
#include "deadline.h"

static struct difference;
float ExecutionTimeCal(struct timespec* start, struct timespec* stop)
{
   
   difference.tv_sec = stop->tv_sec - start->tv_sec;
   difference.tv_nsec = stop->tv_nsec - start->tv_nsec; 
    if (start->tv_nsec > stop->tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec + difference.tv_nsec*0.000000001);
}

float GetTime(struct timespec* value)
{
   difference.tv_sec = value->tv_sec - start.tv_sec;
   difference.tv_nsec = value->tv_nsec - start.tv_nsec; 
    if (start.tv_nsec > value->tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec + difference.tv_nsec*0.000000001);
}

void DisplayAllDeadlines(void)
{
  printf(" Start and Stop times for all threads \n");
  static int i = 0;
  static float maximum;
/*  for(i=0; i< TEST_FRAMES ; i+=1)
  {
    printf("For Read frame\n");
    printf("Frame : %d\n", i);
    printf("Start : %0.3fs\n", exec_time.read_thread[i][START]);
    printf("Stop  : %0.3fs\n", exec_time.read_thread[i][STOP]);
    printf("Execution time : %0.3fs\n", exec_time.read_time[i]);
    

    printf("For Gray conversion\n");
    printf("Frame : %d\n", i);
    printf("Start : %0.3fs\n", exec_time.grey_thread[i][START]);
    printf("Stop  : %0.3fs\n", exec_time.grey_thread[i][STOP]);
    printf("Execution time : %0.3fs\n", exec_time.grey_thread[i][STOP] - exec_time.grey_thread[i][START]);
    

    printf("For dump pgm\n");
    printf("Frame : %d\n",i);
    printf("Start : %fs\n", exec_time.dump_thread[i][START]);
    printf("Stop  : %fs\n", exec_time.dump_thread[i][STOP]);
    printf("Execution time : %fs\n", exec_time.dump_time[i]);
    
    
    printf("-----------------------------------------------\n");
  }
*/
  printf("WCET Read:%f\n",WCET(READ));
  printf("WCET Grey:%f\n",WCET(GREY));
  printf("WCET Dump:%f\n",WCET(DUMP));

}

float WCET(int thread)
{
 static int i = 0;
 float max  =  0.000000;
 for(i=0 ; i<TEST_FRAMES ; i+=1)
 {
   switch(thread)
   {
	case READ :
        
	if(max < exec_time.read_time[i])
	{
	   max = exec_time.read_time[i];
	}
	break;
	case GREY :
	
	if(max < exec_time.grey_time[i] && (exec_time.grey_time[i] < 1.0))
	{
	   max = exec_time.grey_time[i];
	}
	break;
	case DUMP :
	
	if(max < exec_time.dump_time[i])
	{
	   max = exec_time.dump_time[i];
	}
	break;
   }
    
 }
 return max;
}
