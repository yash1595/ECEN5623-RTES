#include "server.h"

float TimeValues(void)
{
    clock_gettime(CLOCK_REALTIME, &finish);
    difference.tv_sec  = finish.tv_sec - start.tv_sec;
    difference.tv_nsec = finish.tv_nsec - start.tv_nsec; 
    if (start.tv_nsec > finish.tv_nsec) { 
                --difference.tv_sec; 
                difference.tv_nsec += nano; 
            } 
    return (difference.tv_sec*1000 + difference.tv_nsec*0.000001);				//ms
}

void print_scheduler(void)                                                      // Displays the scheduling policy.
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is SCHED_FIFO", TimeValues());
           break;
     case SCHED_OTHER:
           syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is SCHED_OTHER",TimeValues()); exit(-1);
       break;
     case SCHED_RR:
           syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is SCHED_RR",TimeValues()); exit(-1);
           break;
     default:
       syslog(LOG_CRIT,"[TIME:%f]Pthread Policy is UNKNOWN",TimeValues()); exit(-1);
   }

}

void SyslogInit(void)
{
    printf("All printf statements will be replaced by syslogs.\n");
    openlog("[640x480]",LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    setlogmask(LOG_UPTO(LOG_DEBUG)); 
    syslog(LOG_INFO,"<<<<<<<<<<<<<<<<<<<<<<<BEGIN>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
}

int SocketInit(void)
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	return sock;
}

int HeadersInit(void)
{
	strncpy(pgm_header, "P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n", sizeof(pgm_header));
	strncpy(pgm_dumpname_grey, "testG00000000.pgm", sizeof(pgm_dumpname_grey));
	pgm_header_len = sizeof(pgm_header);
	return 0;
}

int ConnectToClient(void)
{
	static int opt = 1;
	
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
    	syslog(LOG_ERR,"[TIME:%fms]Set Sock Opt error.", TimeValues());
        return -1;
    } 

    server.sin_family = AF_INET; 
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

	//bind
	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		syslog(LOG_ERR,"[TIME:%fms]Binding error.", TimeValues());
		return -1;
	}

	//Listen
	if(listen(sock, 5) < 0)
	{
		syslog(LOG_ERR,"[TIME:%fms]Listen error.", TimeValues());
		return -1;
	}

	//Accept
	mysock = accept(sock, (struct sockaddr *)0, 0);
	if(mysock == -1)
	{
		syslog(LOG_ERR,"[TIME:%fms]Accept error.", TimeValues());
		return -1;
	}

return 0;

}


void* ReadFrame(void)
{

	static int frame_count_switch = 0, index_val, confirm_val, bytes_received = 0;
    static int frame_confirmation = 999;
 static int frame_time_bytes = 0;
    PROCESS_COMPLETE_BIT = 0;
	syslog(LOG_ERR,"[TIME:%fms]Enter read", TimeValues());
    while(frame_count_switch < TOTAL_FRAMES)
    {
        //usleep(20000);
	valread = 0;
	do
	{
		valread = read(mysock , &frame_conv_time , sizeof(struct timespec));
		if(valread < 0)
		{
			syslog(LOG_ERR,"[TIME:%fms]Read value issue", TimeValues());
		}
		else
		{
		  frame_time_bytes += valread;
		}
		
	}
	while(frame_time_bytes < sizeof(struct timespec));

	index_val = 0;
	bytes_received = 0;
	valread = 0;
	syslog(LOG_ERR,"[TIME:%fms]frame_count:%d", TimeValues(), frame_count_switch);
    	do
    	{
    		valread = read(mysock , &split_array[index_val], INDEX_2);
    		if(valread < 0)
    		{
    			syslog(LOG_ERR,"[TIME:%fms]Read value issue index_val", TimeValues());
    		}
		else
		{
		  index_val+=valread;
		  bytes_received+=valread;
		  
		}
	
    	}while(bytes_received<IMG_SIZE);
	syslog(LOG_ERR,"[TIME:%fms]bytes_received:%d", TimeValues(), bytes_received);


	
/************************************* rec-send-2 ****************************************************/	

	snprintf(&pgm_dumpname_grey[4], 9, "%08d", frame_count_switch);
	strncat(&pgm_dumpname_grey[12], ".pgm", 5);
	snprintf(&pgm_header[4], 11, "%010d", (int)frame_conv_time.tv_sec); //frame_conv_time->tv_sec
	strncat(&pgm_header[14], " sec ", 5);
	snprintf(&pgm_header[19], 11, "%010d", (int)(frame_conv_time.tv_nsec/1000000)); //(frame_conv_time->tv_nsec)/1000000

	dumpfd = open(pgm_dumpname_grey, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

	strncat(&pgm_header[29], " msec \n"H640" "V480"\n255\n", 19);

	written=write(dumpfd, pgm_header, pgm_header_len);
	    
	    total=0;
	    index_val=0;
	    do
	    {
	      written=write(dumpfd, &split_array[index_val], 307200);
	      if(written > 0)
	      {
		 total+=written;
	      	 index_val+=written;
	      }
	     
	    } while(total < (307200));

	    close(dumpfd);
	
	confirm_val = send(mysock, &frame_count_switch, sizeof(int), 0);
	if(confirm_val < 0)
	{
	  syslog(LOG_ERR,"[TIME:%fms]Confirm send error", TimeValues());
	}

	frame_count_switch+=1;    
    }
    PROCESS_COMPLETE_BIT = 1;
}
	   


