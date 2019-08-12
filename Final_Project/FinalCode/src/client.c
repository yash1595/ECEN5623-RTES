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

int ReadFrameCount;

// Initiate Socket transfer

int SocketInit(void)
{  
	retry_count = 0;
	socket_read = 99;
	StartBit = 0;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	return sock;
}

int ConnectToServer(void)
{

    server_ptr = gethostbyname(SERVER_ADDR);
    server.sin_family = AF_INET; 
    server.sin_port = htons(PORT);
    memcpy(&server.sin_addr, server_ptr->h_addr, server_ptr->h_length);

	while(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0 && (retry_count < RETRY_COUNT))
	{
		retry_count+=1;
	}

	if(retry_count == RETRY_COUNT)
	{
		return -1;
	}

	return 0;
}

void* SendToServer(void)
{


	static int index_val = 0, send_rt, bytes_sent = 0;
	static int frame_confirmation = 0, new_frame_confirmation = 90, frame_count = 0;
	syslog(LOG_INFO,"ReadFrameCount:%d",ReadFrameCount);
	SOCKET_COMPLETE = 0;
	socket_read = 99;

	while(SOCKET_COMPLETE != 1)
	{

		if(((socket_read+1)%100 == circ_write))continue; 
		socket_read = (socket_read + 1) % 100;
		memcpy(split_array, CIRCULAR_BUFFER[socket_read].ptr, (CIRCULAR_BUFFER[socket_read].SIZE)); 
		send_rt = 0;
		bytes_sent = 0;
		do
		{
			send_rt = send(sock, CIRCULAR_BUFFER[socket_read].frame_conv_time, sizeof(struct timespec), 0);
			if(send_rt > 0)
			{
			  bytes_sent += send_rt;
			}
		}
		while(bytes_sent < sizeof(struct timespec));

		//send_rt = send(sock, CIRCULAR_BUFFER[socket_read].frame_conv_time, sizeof(struct timespec), 0);		

		send_rt = 0;
		index_val=0;
		bytes_sent = 0;
		do
		{
			//syslog(LOG_INFO, "index:%d",index_val);
			send_rt = send(sock, &split_array[index_val], (INDEX_2), 0);
			if(send_rt < 0)
			{
			  syslog(LOG_INFO, "Sent rt:%d",send_rt);
			}
			else
			{
			  index_val+=send_rt;
			  bytes_sent+=send_rt;
			}
		}
		while(bytes_sent<CIRCULAR_BUFFER[socket_read].SIZE);

		

		do
		{
			read(sock, &new_frame_confirmation, sizeof(int));
			syslog(LOG_INFO, "Read frame_confirmation:%d Frame confirmation: %d",new_frame_confirmation, frame_confirmation);
		}			
		while(frame_confirmation !=  new_frame_confirmation);
		frame_confirmation += 1 ;	
		frame_count+=1;
		if(frame_confirmation >= TotalFrames)SOCKET_COMPLETE = 1;	
	
		
	}
	return 0;
}
 
