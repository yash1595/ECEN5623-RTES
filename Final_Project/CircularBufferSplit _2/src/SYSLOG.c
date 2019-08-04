#include "SYSLOG.h"

void SyslogInit(void)
{
    printf("All printf statements will be replaced by syslogs.\n");
    //printf("DEADLINE is taken as 2s for %d frames.\n",TotalFrames);
    openlog("[640x480]",LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    setlogmask(LOG_UPTO(LOG_DEBUG)); 
    syslog(LOG_INFO,"<<<<<<<<<<<<<<<<<<<<<<<BEGIN>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
}

float TimeValues(void)
{
    clock_gettime(CLOCK_REALTIME, &finish);
    difference.tv_sec  = finish.tv_sec - start.tv_sec;
    difference.tv_nsec = finish.tv_nsec - start.tv_nsec; 
    if (start.tv_nsec > finish.tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec + difference.tv_nsec*0.000000001);
}



/*
typedef struct 
{
	 int frame_index;
	 struct timespec* grey_thread[TEST_FRAMES][2];
	 struct timespec* read_thread[TEST_FRAMES][2];
	 struct timespec* dump_thread[TEST_FRAMES][2];
	 float grey_time[TEST_FRAMES];
	 float read_time[TEST_FRAMES];
	 float dump_time[TEST_FRAMES];
}exectimes;
*/


