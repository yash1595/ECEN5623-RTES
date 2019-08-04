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

int HRES[5]={160,320,640,800,960};
int VRES[5]={120,240,480,600,720};


int fd = -1;
struct buffer *buffers;
unsigned int n_buffers;
int out_buf;
int force_format=1;
int frame_count = 30;
unsigned char gray_ptr[(1280*960)];

unsigned int framecnt=0;

char* converison_frames[]={"GREY","BRIGHT","CONTRAST"};


int main(int argc, int *argv[])
{
	SocketSet = false;
	if(argc > 1)
	{
		
		  if(strncmp(argv[1], _10HZ_, sizeof(_10HZ_)) == 0)
		  {
		  	CAPTURE_RATE = 10;
		  	printf("<<<<<<<<<<<<<<<<<<<Will capture frames at 10Hz : %d>>>>>>>>>>>>>>>>>>>>>>>>\n", CAPTURE_RATE);
		  }
		  else
		  {
		    	CAPTURE_RATE = 1;	
		  	printf("<<<<<<<<<<<<<<<<<<<Will capture frames at 1Hz : %d>>>>>>>>>>>>>>>>>>>>>>>>\n", CAPTURE_RATE); 
		  }
		  if(argc-2 > 0)
		  {
			  if(strncmp(argv[2], SOCKET, sizeof(SOCKET)) == 0)
			  {
			  	SocketSet = true;
			  	printf("<<<<<<<<<<<<<<<<<<<Sockets will be used>>>>>>>>>>>>>>>>>>>>>>>>\n", CAPTURE_RATE);
			  }
			
			  else
			  {
			 	SocketSet = false;	
			 	printf("<<<<<<<<<<<<<<<<<<<No sockets>>>>>>>>>>>>>>>>>>>>>>>>\n", CAPTURE_RATE); 
			  }
	          }	
	}
	else
	{
		CAPTURE_RATE = 1;
		SocketSet = false;
		printf("<<<<<<<<<<<<<<<<<<<Will capture frames at 1Hz : %d>>>>>>>>>>>>>>>>>>>>>>>>\n", CAPTURE_RATE);
		printf("<<<<<<<<<<<<<<<<<<<No sockets>>>>>>>>>>>>>>>>>>>>>>>>\n", CAPTURE_RATE);
	}
       
        frame_rate=0.0; 
        resolution = 2;
        

        PROCESS_COMPLETE_BIT=0;
        
        clock_gettime(CLOCK_REALTIME, &start);

        SyslogInit();

        if(sem_init(&semA,0,0) || sem_init(&semB,0,0))
        {
		      syslog(LOG_ERR,"[TIME:%f]Sem init error", TimeValues());
        }
if(SocketSet == true)
{
        if(SocketInit() == -1)
        {
          syslog(LOG_ERR,"[TIME:%fms]Socket init error.", TimeValues());
          exit(1);
        }

        if(ConnectToServer() == -1)
        {
          syslog(LOG_ERR,"[TIME:%fms]Socket Connect error.", TimeValues());
          exit(1);
        }
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

  	
	 //FrameCorrection();
	
	

       for(common_struct_index = 0; common_struct_index< TEST_FRAMES; common_struct_index+=1)
       {
	if(FrameCheck()==-1)
	    {
  		exit(1);
	    }
	 FrameDelay();    
	 StoreFrameCheck(); 
	}

	 //StoreFrameCheck();

         if(DisplayFrameDiff()==-1)
         {
           exit(1);
         }

	 ReadFrameCount = 0;

    /****************************************************************************************/
        // CPU_ZERO(&allcpuset);

        // for(i=0; i < NUM_CPU_CORES; i++)
        // {
        //    CPU_SET(i, &allcpuset);
        // }
        // syslog(LOG_CRIT,"Using CPUS=%d from total available.", CPU_COUNT(&allcpuset));

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

if(SocketSet == true)
{
	  CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(2, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[4]);                                  // Set up attributes for setting priroity vi FIFO.

          rc=pthread_attr_setinheritsched(&rt_sched_attr[4], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[4], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[4], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-4;
          pthread_attr_setschedparam(&rt_sched_attr[4], &rt_param[4]);

     //     threadParams[i].threadIdx=i;
          
          rc=pthread_create(&threads[4],               // pointer to thread descriptor
                            &rt_sched_attr[4],         // use specific attributes
                            //(void *)0,                 // default attributes
                            function_pointer[4] ,                     // thread function entry point
                            0 // parameters to pass in
                            );
	

}	
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
                            function_pointer[i] ,                     // thread function entry point
                            0 // parameters to pass in
                            );

        }

         


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
                            function_pointer[0] ,                     // thread function entry point
                            0 // parameters to pass in
                            );
  //        usleep(1000);

        



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
                            function_pointer[3] ,                     // thread function entry point
                            0 // parameters to pass in
                            );
  //        usleep(1000);
	  
	  

        

       
	 
	       while(PROCESS_COMPLETE_BIT == 0);    
		//dump_pgm();
	       for(i=0;i<NUM_THREADS;i+=1)
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

        //syslog(LOG_INFO,"[TIME:%fus]Completed Reading frame.", TimeValues());
        DisplayAllDeadlines();
        printf("In order to see the syslog, type the following: cat /var/log/syslog | grep '640x480' | grep 'Scheduler thread'\n");
    
	/***************************************************************************************/   
	
    //DisplayDeadlines();

    return 0;
}

