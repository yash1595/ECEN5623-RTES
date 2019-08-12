#include "SYSLOG.h"

/****************************************************************
*@brief : Function initializes syslog ro replace printf
*@params: None
*@return: None
****************************************************************/
void SyslogInit(void)
{
    printf("All printf statements will be replaced by syslogs.\n");
    openlog("[640x480]", LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    setlogmask(LOG_UPTO(LOG_DEBUG)); 
    syslog(LOG_INFO,"<<<<<<<<<<<<<<<<<<<<<<<BEGIN>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
}

/****************************************************************
*@brief : Function gets the current time in the system
*@params: None
*@return: Current value of time in seconds
****************************************************************/
float TimeValues(void)
{
    clock_gettime(CLOCK_REALTIME, &finish);
    difference.tv_sec  = finish.tv_sec - start.tv_sec;
    difference.tv_nsec = finish.tv_nsec - start.tv_nsec; 
    if (start.tv_nsec > finish.tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec + difference.tv_nsec*DecNano);
}

float TimeValuesMicro(void)
{
    clock_gettime(CLOCK_REALTIME, &finish);
    difference.tv_sec  = finish.tv_sec - start.tv_sec;
    difference.tv_nsec = finish.tv_nsec - start.tv_nsec; 
    if (start.tv_nsec > finish.tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec*1000000 + difference.tv_nsec*0.001);
}


