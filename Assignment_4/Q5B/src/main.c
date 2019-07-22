/*
 *
 *  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  grabber NTSC cameras to acquire digital video from a source,
 *  time-stamp each frame acquired, save to a PGM or PPM file.
 *
 *  The original code adapted was open source from V4L2 API and had the
 *  following use and incorporation policy:
 * 
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 * Similar Code: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html?highlight=fd_set
 */

#include "main.h"

int HRES[5]={160,320,640,800,960};
int VRES[5]={120,240,480,600,720};

char* dev_name;
enum io_method io = IO_METHOD_MMAP;
int fd = -1;
struct buffer *buffers;
unsigned int n_buffers;
int out_buf;
int force_format=1;
int frame_count = 30;
unsigned char gray_ptr[(1280*960)];

char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_dumpname_grey[]="testG00000000.pgm";
char pgm_dumpname_bright[]="testB00000000.pgm";
char pgm_dumpname_contrast[]="testC00000000.pgm";

unsigned int framecnt=0;

unsigned int pgm_header_len = sizeof(pgm_header);

char* converison_frames[]={"GREY","BRIGHT","CONTRAST"};


int main(void)
{

        frame_rate=0.0; 
        resolution = 0 	;
        SchedulerIndicate  = 1;

        PROCESS_COMPLETE_BIT=0;
        
        clock_gettime(CLOCK_REALTIME, &start);

	if(pthread_mutex_init(&MUTEX,NULL))
	  printf("Mutex init failed\n");

        SyslogInit();

        if(sem_init(&semA,0,0) || sem_init(&semB,0,0) || sem_init(&semC,0,0))
        {
		      syslog(LOG_ERR,"[TIME:%f]Sem init error", TimeValues());
        }

        if(OpenDevice()==-1)
        {
          exit(1);
        }

        if(InitDevice()==-1)
        {
          exit(1);
        }

        if(InitThreads()==-1)
        {
          exit(1);
        }
        
        if(StartCapturing()==-1)
        {
		      exit(1);
        }

    /****************************************************************************************/

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

        syslog(LOG_CRIT,"[TIME:%f]rt_max_prio=%Ld", TimeValues(),rt_max_prio);
        syslog(LOG_CRIT,"[TIME:%f]rt_min_prio=%ld", TimeValues(),rt_min_prio);

	

        for(i=2; i < NUM_THREADS ; i+=1)                                            // Schedules all 3 tasks in RM scheduing.
        {
          CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(3, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[i]);                                  // Set up attributes for setting priroity vi FIFO.
          rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-i;
          pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
          
          rc=pthread_create(&threads[i],                                            // pointer to thread descriptor
                            &rt_sched_attr[i],                                      // use specific attributes
                            //(void *)0,                                            // default attributes
                            function_pointer[i] ,                                   // thread function entry point
                            0 // parameters to pass in
                            );

        }

	 for(i=0; i < 2 ; i+=1)                                                           // Initializes Scheduler and ReadFrame task. 
        {
          CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(0, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[i]);                                  // Set up attributes for setting priroity vi FIFO.
          rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-i;
          pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
          
          rc=pthread_create(&threads[i],                                          // pointer to thread descriptor
                            &rt_sched_attr[i],                                    // use specific attributes
                            //(void *)0,                                          // default attributes
                            function_pointer[i] ,                                 // thread function entry point
                            0 // parameters to pass in
                            );

        }

       
	 
	       while(PROCESS_COMPLETE_BIT == 0);                                           // Program stalls till PROCESS_COMPLETE_BIT == 0.

	       for(i=0;i<NUM_THREADS;i+=1)                                                 // All thread attributes are destroyed.
          {
              rc=pthread_attr_destroy(&rt_sched_attr[i]); 
          }

        for(i=0;i<NUM_THREADS;i+=1)                                                   // Terminate the threads.
          {
              pthread_join(threads[i], NULL);
          }



       	if(UninitDevice()==-1)
        	{
        		exit(1);
        	}
      	if(CloseDevice()==-1)
        	{
        		exit(1);
        	}
      	if(StopCapturing()==-1)
        	{
        		exit(1);
        	}

        DisplayAllDeadlines();
        printf("In order to see the syslog, type the following: cat /var/log/syslog | grep '960x720' | grep 'Scheduler thread'\n");
    
	/***************************************************************************************/   
	
    DisplayDeadlines();

    return 0;
}

