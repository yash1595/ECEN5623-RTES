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
char pgm_dumpname[]="test00000000.pgm";

char ppm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char ppm_dumpname[]="test00000000.ppm";

unsigned int framecnt=0;

unsigned int pgm_header_len = sizeof(pgm_header);
unsigned int ppm_header_len = sizeof(ppm_header);
char* converison_frames[]={"GREY","BRIGHT","CONTRAST"};


int main(void)
{

        frame_rate=0.0;
        clock_gettime(CLOCK_REALTIME, &start);
        SyslogInit();
    for(resolution = 0; resolution < 1; resolution += 1)
    {
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
    /****************************************************************************************/
        CPU_ZERO(&allcpuset);

        for(i=0; i < NUM_CPU_CORES; i++)
        {
           CPU_SET(i, &allcpuset);
        }
        syslog(LOG_CRIT,"Using CPUS=%d from total available.", CPU_COUNT(&allcpuset));

        mainpid=getpid();

        rt_max_prio = sched_get_priority_max(SCHED_FIFO);                           // Priorities for the tasks (max,min)
        rt_min_prio = sched_get_priority_min(SCHED_FIFO);

        main_param.sched_priority=rt_max_prio;
        rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);                   // Scheduler is set to FIFO.
        if(rc < 0) perror("main_param");

        print_scheduler();

        syslog(LOG_CRIT,"rt_max_prio=%d", rt_max_prio);
        syslog(LOG_CRIT,"rt_min_prio=%d", rt_min_prio);

        for(i=0; i < NUM_THREADS; i+=1)
        {
          CPU_ZERO(&threadcpu);                                                     // Ensures each thread executes on only 1 CPU core and it is the same.
          CPU_SET(3, &threadcpu); 

          rc=pthread_attr_init(&rt_sched_attr[i]);                                  // Set up attributes for setting priroity vi FIFO.
          rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED); //Inheritance of the tasks is set.
          rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);            // Scheduling is set as FIFO.
          rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

          rt_param[i].sched_priority=rt_max_prio-i;
          pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

          threadParams[i].threadIdx=i;

          rc=pthread_create(&threads[i],               // pointer to thread descriptor
                            &rt_sched_attr[i],         // use specific attributes
                            //(void *)0,                 // default attributes
                            function_pointer[i] ,                     // thread function entry point
                            0 // parameters to pass in
                            );
	if(resolution < 3)
		sleep(3);
        else
	        sleep(12);
        }
        //while(PROCESS_COMPLETE_BIT!=1);

        for(i=0;i<NUM_THREADS;i+=1)
        {
            rc=pthread_attr_destroy(&rt_sched_attr[i]); 
        }

        for(i=0;i<NUM_THREADS;i+=1)                                                   // Terminate the threads.
        {
            pthread_join(threads[i], NULL);
        }
        syslog(LOG_INFO,"[TIME:%fus]Completed Reading frame.", TimeValues());
        printf("In order to see the syslog, type the following: cat /var/log/syslog | grep '5-B'\n");
    
	/***************************************************************************************/   
	if(UninitDevice()==-1)
	{
	exit(1);
	}
	if(CloseDevice()==-1)
	{
	exit(1);
	}
    }
    //DisplayDeadlines();
    return 0;
}

