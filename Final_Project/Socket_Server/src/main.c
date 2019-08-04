#include "server.h"

int main(void)
{
	
	clock_gettime(CLOCK_REALTIME, &start);

	SyslogInit();

	if(SocketInit() == -1)
	{
		syslog(LOG_ERR,"[TIME:%fms]Socket not initilaized", TimeValues());
	}

	if(HeadersInit() != 0)
	{
		exit(1);
	}

	if(ConnectToClient() != 0)
	{
		exit(1);
	}

	mainpid=getpid();

	rt_max_prio = sched_get_priority_max(SCHED_FIFO);                           // Priorities for the tasks (max,min)
	rt_min_prio = sched_get_priority_min(SCHED_FIFO);

	main_param.sched_priority=rt_max_prio;
	rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);                   // Scheduler is set to FIFO.
	if(rc < 0)
	{
	   syslog(LOG_ERR,"[TIME:%f]Main Param",TimeValues());
	}

	print_scheduler();

	CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
	CPU_SET(3, &threadcpu); 

	rc=pthread_attr_init(&rt_sched_attr[0]);                                  // Set up attributes for setting priroity vi FIFO.
	rc=pthread_attr_setinheritsched(&rt_sched_attr[0], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
	rc=pthread_attr_setschedpolicy(&rt_sched_attr[0], SCHED_FIFO);            // Scheduling is set as FIFO.
	rc=pthread_attr_setaffinity_np(&rt_sched_attr[0], sizeof(cpu_set_t), &threadcpu);

	rt_param[0].sched_priority=rt_max_prio-0;
	pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);

	//     threadParams[i].threadIdx=i;

	rc=pthread_create(&capture_frames,               // pointer to thread descriptor
	                  &rt_sched_attr[0],         // use specific attributes
	                //(void *)0,                 // default attributes
	                  ReadFrame ,                     // thread function entry point
	                  0 // parameters to pass in
	                );

	while(PROCESS_COMPLETE_BIT == 0);    
	//dump_pgm();

	rc=pthread_attr_destroy(&rt_sched_attr[0]); 

	pthread_join(capture_frames, NULL);

	return 0;
}



