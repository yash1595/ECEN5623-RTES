#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_dump_pgm.h"
#include "thread_init.h"

/*******************************************************************************
*@brief  : Stores images in local storage
*@params : None
*@return : None
*******************************************************************************/
void* dump_pgm(void)
{
    char pgm_dumpname_grey[]="01HZG00000000.pgm";

    if(CAPTURE_RATE == _10HZ_MODE)
    {
	strncpy(pgm_dumpname_grey, "10HZG00000000.pgm", sizeof(pgm_dumpname_grey));
    }
		
    char pgm_header[]="P5\n#9999999999 sec 9999999999 msec yash-desktop 4.9.140-tegra\n"HRES_STR" "VRES_STR"\n255\n";
    
    int written,total, dumpfd;    
                            
    while(PROCESS_COMPLETE_BIT != 1)								//While process is not complete
    {
	
        if((circ_read+1)%100 == circ_write) continue;						//Runs only when read pointer is not equal to write pointer
	
	syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Start time pthread_dump]",TimeValues());		//Displays start time of dump
	clock_gettime(CLOCK_REALTIME, &start_dump_pgm);
	
        circ_read = (circ_read+1)%100;								//Increment read pointer

 	if(CIRCULAR_BUFFER[circ_read].frame_count%100 == 0)					//Stores time of 100 frames
	{
		syslog(LOG_INFO,"[TIME:%fus]Check point %d",TimeValues(), CIRCULAR_BUFFER[circ_read].frame_count);
	}
	syslog(LOG_INFO,"[TIME:%0.3fs]FRAME DUMP:%d",TimeValues(), CIRCULAR_BUFFER[circ_read].frame_count);
	snprintf(&pgm_dumpname_grey[4], 9, "%08d", CIRCULAR_BUFFER[circ_read].frame_count);	//Populates the header name
	strncat(&pgm_dumpname_grey[12], ".pgm", 5);						//Populates the header name
	dumpfd = open(pgm_dumpname_grey, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);		//Opens the file with header name

        snprintf(&pgm_header[4], 11, "%010d", (int)CIRCULAR_BUFFER[circ_read].frame_conv_time->tv_sec); //Populates the header name
        strncat(&pgm_header[14], " sec ", 5);								//Populates the header name
        snprintf(&pgm_header[19], 11, "%010d", (int)(CIRCULAR_BUFFER[circ_read].frame_conv_time->tv_nsec)/1000000); //Populates the header name with time of capture

        strncat(&pgm_header[29], " msec ", 6);							//Populates the header name 
	strncat(&pgm_header[35],"yash-desktop 4.9.140-tegra\n"H640" "V480"\n255\n", 62);	//Populates the header name with uname

        written=write(dumpfd, pgm_header, sizeof(pgm_header));					//Writes header name in file descriptor
        total=0;

        do
        {
          written=write(dumpfd, CIRCULAR_BUFFER[circ_read].ptr, CIRCULAR_BUFFER[circ_read].SIZE); //Writes image into local storage
          total+=written;
        } while(total < CIRCULAR_BUFFER[circ_read].SIZE);
	clock_gettime(CLOCK_REALTIME, &stop_dump_pgm);
	exec_time.dump_time[independent] = ExecutionTimeCal(&start_dump_pgm,&stop_dump_pgm);	//Stores execution time in global structure
	
	 syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Stop  time pthread_dump]",TimeValues());		//Displays stop time of dump thread
         syslog(LOG_INFO,"[TIME:%0.3fs]!![JITTER Diff  time pthread_dump] %0.3fs",TimeValues(),exec_time.dump_time[independent]); //Displays execution time of dump thread
    close(dumpfd);										//CLoses file descriptor
         
    }
  pthread_exit(0);//return 0;
}

