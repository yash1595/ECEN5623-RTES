#include "main.h"
#include "client.h"
#include "SYSLOG.h"
#include "pthread_dump_pgm.h"
#include "pthread_gray.h"
#include "pthread_read_frame.h"
#include "pthread_scheduler.h"
#include "thread_init.h"


int InitThreads(void)
{
    function_pointer[0]=Scheduler;                        //CORE-1
    function_pointer[1]=ReadFrame;                        //CORE-3

    function_pointer[2]=pthread_grey;                     //CORE-3

    function_pointer[3]=dump_pgm;                         //CORE-2
    function_pointer[4]=SendToServer;

    return 0;
}


/**********************************************************************************************/





