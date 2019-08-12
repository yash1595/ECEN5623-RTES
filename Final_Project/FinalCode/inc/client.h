#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "main.h"
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <netdb.h>

#define PORT 8080
#define SERVER_ADDR	"192.168.50.104"
#define RETRY_COUNT	(5)
#define INDEX_1		(75)
#define INDEX_2		(4096)

struct hostent *server_ptr;
struct COMMON_IMG_PROC socket_send;
//variables
int sock, receive;
int retry_count, socket_read;
struct sockaddr_in server;
int StartBit;
int SOCKET_COMPLETE;

unsigned char split_array[IMG_SIZE];

//Functions
int SocketInit(void);
int ConnectToServer(void);
void* SendToServer(void);

#endif
