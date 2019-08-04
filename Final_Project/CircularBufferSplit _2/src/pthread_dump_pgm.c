#include "SYSLOG.h"
#include "deadline.h"
#include "pthread_dump_pgm.h"
#include "thread_init.h"

void LoadHeaderNames(void)
{
  
  syslog(LOG_INFO,"[TIME:%f]>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Length:%d", TimeValues(), sizeof("yash-desktop 4.9.140-tegra"));
}

void* dump_pgm(void)
{
	
    char pgm_header[]="P5\n#9999999999 sec 9999999999 msec yash-desktop 4.9.140-tegra\n"HRES_STR" "VRES_STR"\n255\n";
    char pgm_dumpname_grey[]="testG00000000.pgm";
    int written,total, dumpfd,queue_bytes;                             
    while(PROCESS_COMPLETE_BIT != 1)
    {
	
        if((circ_read+1)%100 == circ_write) continue;
	
	syslog(LOG_INFO,"[TIME:%0.3fs]!![Start time pthread_dump]",TimeValues());
	clock_gettime(CLOCK_REALTIME, &start_dump_pgm);
	exec_time.dump_thread[OldframeCount][START]=GetTime(&start_dump_pgm);

        circ_read = (circ_read+1)%100;
        syslog(LOG_INFO,"[TIME:%fus]Receive Thread",TimeValues());

 	if(CIRCULAR_BUFFER[circ_read].frame_count%100 == 0)
		syslog(LOG_INFO,"[TIME:%fus]6000 check %d",TimeValues(), CIRCULAR_BUFFER[circ_read].frame_count);
	

      	//syslog(LOG_INFO,"[TIME:%fus]circ_read:%d.",TimeValues(), circ_read);
	snprintf(&pgm_dumpname_grey[4], 9, "%08d", CIRCULAR_BUFFER[circ_read].frame_count);
	strncat(&pgm_dumpname_grey[12], ".pgm", 5);
	dumpfd = open(pgm_dumpname_grey, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

        snprintf(&pgm_header[4], 11, "%010d", (int)CIRCULAR_BUFFER[circ_read].frame_conv_time->tv_sec);
        strncat(&pgm_header[14], " sec ", 5);
        snprintf(&pgm_header[19], 11, "%010d", (int)(CIRCULAR_BUFFER[circ_read].frame_conv_time->tv_nsec)/1000000);

        strncat(&pgm_header[29], " msec ", 6);
	strncat(&pgm_header[35],"yash-desktop 4.9.140-tegra\n"H640" "V480"\n255\n", 62);

        written=write(dumpfd, pgm_header, sizeof(pgm_header));
        total=0;

        do
        {
          written=write(dumpfd, CIRCULAR_BUFFER[circ_read].ptr, CIRCULAR_BUFFER[circ_read].SIZE);
          total+=written;
        } while(total < CIRCULAR_BUFFER[circ_read].SIZE);
	clock_gettime(CLOCK_REALTIME, &stop_dump_pgm);

	
	 syslog(LOG_INFO,"[TIME:%0.3fs]!![Stop  time pthread_dump]",TimeValues());
         syslog(LOG_INFO,"[TIME:%0.3fs]!![Diff  time pthread_dump] : %fs",TimeValues(),  ExecutionTimeCal(&start_dump_pgm,&stop_dump_pgm));
	 exec_time.dump_time[independent]= ExecutionTimeCal(&start_dump_pgm,&stop_dump_pgm);
    close(dumpfd);
         
    }
 
}

