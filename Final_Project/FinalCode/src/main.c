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
#include "client.h"
#include "SYSLOG.h"
#include "camera.h"
#include "deadline.h"
#include "nonblur.h"
#include "pthread_dump_pgm.h"
#include "pthread_gray.h"
#include "pthread_read_frame.h"
#include "pthread_scheduler.h"
#include "thread_init.h"

int fd = -1;
struct buffer *buffers;
unsigned int n_buffers;
int out_buf;
int force_format=1;
int frame_count = 30;
unsigned char gray_ptr[(1280*960)];

unsigned int framecnt=0;

char* converison_frames[]={"GREY","BRIGHT","CONTRAST"};


int main(int argc, char **argv)
{       OldframeCount = 0;
	SocketSet = false;
	HRES = 640;
	VRES = 480;
	if(argc > 1)
	{
		
		  if(strncmp(argv[1], _10HZ_, sizeof(_10HZ_)) == 0)	//Checks for frequency
		  {
		  	CAPTURE_RATE = _10HZ_MODE;
		  	printf("<<<<<<<<<<<<<<<<<<<Will capture frames at 10Hz>>>>>>>>>>>>>>>>>>>>>>>>\n");
		  }
		  else
		  {
		    	CAPTURE_RATE = _1HZ_MODE;	
		  	printf("<<<<<<<<<<<<<<<<<<<Will capture frames at 1Hz>>>>>>>>>>>>>>>>>>>>>>>>\n"); 
		  }
		  if(argc-2 > 0)					//Checks for sockets
		  {
			  if(strncmp(argv[2], SOCKET, sizeof(SOCKET)) == 0)
			  {
			  	SocketSet = true;
			  	printf("<<<<<<<<<<<<<<<<<<<Sockets will be used>>>>>>>>>>>>>>>>>>>>>>>>\n");
			  }
			
			  else
			  {
			 	SocketSet = false;	
				SOCKET_COMPLETE = 1;
			 	printf("<<<<<<<<<<<<<<<<<<<No sockets>>>>>>>>>>>>>>>>>>>>>>>>\n"); 
			  }
	          }	
	}
	else
	{
		CAPTURE_RATE = _1HZ_MODE;
		SocketSet = false;
		printf("<<<<<<<<<<<<<<<<<<<Will capture frames at 1Hz>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("<<<<<<<<<<<<<<<<<<<No sockets>>>>>>>>>>>>>>>>>>>>>>>>\n");
	}
       
        frame_rate=0.0; 
        PROCESS_COMPLETE_BIT=0;
        
        clock_gettime(CLOCK_REALTIME, &start);

        SyslogInit();								//Initializes the syslog

        if(sem_init(&semRead,0,0) || sem_init(&semGray,0,0))			//Initializes semaphores
        {
		      syslog(LOG_ERR,"[TIME:%f]Sem init error", TimeValues());
        }
if(SocketSet == true)								//If sockets are used
{
        if(SocketInit() == -1)							//Initialize sockets
        {
          syslog(LOG_ERR,"[TIME:%fms]Socket init error.", TimeValues());
          exit(1);
        }

        if(ConnectToServer() == -1)						//Connect to server
        {
          syslog(LOG_ERR,"[TIME:%fms]Socket Connect error.", TimeValues());
          exit(1);
        }
}
        if(OpenDevice()==-1)							//Opens camera
        {
          exit(1);
        }

        if(InitDevice()==-1)							//Initializes the camera
        {
          exit(1);
        }

        if(InitThreads()==-1)							//Assign threads to function_pointer
        {
          exit(1);
        }
        
        if(StartCapturing()==-1)						//Begins the capturing process
        {
	  exit(1);
        }

       for(common_struct_index = 0; common_struct_index< TEST_FRAMES; common_struct_index+=1)
       {
		if(FrameCheck()==-1)							//Captures images for blur check
		    {
	  		exit(1);
		    }
		 FrameDelay();    							//Provides for a 100ms delay
		 StoreFrameCheck(); 
	}

         if(DisplayFrameDiff()==-1)						//Checks for blur frame
         {
           exit(1);
         }

	 ReadFrameCount = 0;

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

        syslog(LOG_CRIT,"[TIME:%f]rt_max_prio=%d", TimeValues(),rt_max_prio);	   //Displays maximum and minimum priorities
        syslog(LOG_CRIT,"[TIME:%f]rt_min_prio=%d", TimeValues(),rt_min_prio);


	if(SocketSet == true)							   // Sets up socket thread is sockets are specified
	{
	  CPU_ZERO(&threadcpu);                                                    // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(0, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[4]);                                 // Set up attributes for setting priroity vi FIFO.

          rc=pthread_attr_setinheritsched(&rt_sched_attr[4], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[4], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[4], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-4;
          pthread_attr_setschedparam(&rt_sched_attr[4], &rt_param[4]);

     //     threadParams[i].threadIdx=i;
          
          rc=pthread_create(&threads[4],               // pointer to thread descriptor
                            &rt_sched_attr[4],         // use specific attributes
                            //(void *)0,                 // default attributes
                            (void*)function_pointer[4] ,                     // thread function entry point
                            0 // parameters to pass in
                            );
	

	}

/* Sets the Read frame and Gray conversion thread */
        for(i=1; i < 3 ; i+=1)
        {
          CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(3, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[i]);                                  // Set up attributes for setting priroity vi FIFO.
          rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-i;
          pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

       //   threadParams[i].threadIdx=i;
          
          rc=pthread_create(&threads[i],               // pointer to thread descriptor
                            &rt_sched_attr[i],         // use specific attributes
                            //(void *)0,                 // default attributes
                            (void*)function_pointer[i] ,                     // thread function entry point
                            0 // parameters to pass in
                            );

        }

         
/* Sets the scheduler thread */

          CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(3, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[0]);                                  // Set up attributes for setting priroity vi FIFO.
          rc=pthread_attr_setinheritsched(&rt_sched_attr[0], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[0], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[0], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-0;
          pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);

     //     threadParams[i].threadIdx=i;
          
          rc=pthread_create(&threads[0],               // pointer to thread descriptor
                            &rt_sched_attr[0],         // use specific attributes
                            //(void *)0,                 // default attributes
                            (void*)function_pointer[0] ,                     // thread function entry point
                            0 // parameters to pass in
                            );

/* Sets the Dump thread */
          CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(1, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[3]);                                  // Set up attributes for setting priroity vi FIFO.
          rc=pthread_attr_setinheritsched(&rt_sched_attr[3], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[3], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[3], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-3;
          pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);

     //     threadParams[i].threadIdx=i;
          
          rc=pthread_create(&threads[3],               // pointer to thread descriptor
                            &rt_sched_attr[3],         // use specific attributes
                            //(void *)0,                 // default attributes
                            (void*)function_pointer[3] ,                     // thread function entry point
                            0 // parameters to pass in
                            );
 
          while(PROCESS_COMPLETE_BIT == 0);    

	  for(i=0;i<NUM_THREADS;i+=1)							//Destroys the threads
          {
              rc=pthread_attr_destroy(&rt_sched_attr[i]); 
          }

          for(i=0;i<NUM_THREADS;i+=1)                                                   // Terminate the threads.
          {
              pthread_join(threads[i], NULL);
          }

       	if(UninitDevice()==-1)								//Uninitializes the device
	{
		exit(1);
	}
      	if(CloseDevice()==-1)								//Closes the device
	{
		exit(1);
	}
      	if(StopCapturing()==-1)								//Stops capturing on the device
	{
		exit(1);
	}

        DisplayAllDeadlines();

        printf("In order to see the syslog type the following: cat /var/log/syslog | grep '640x480'\n");
	printf("followed by:\n");
        printf("| grep 'time pthread_scheduler'\n");
	printf("| grep 'time pthread_read'\n");
	printf("| grep 'time pthread_gray'\n");
	printf("| grep 'time pthread_dump'\n");

    return 0;
}

