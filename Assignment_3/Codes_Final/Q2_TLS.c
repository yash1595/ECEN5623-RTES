#include<stdio.h> 
#include<string.h> 
#include<pthread.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include <time.h> 
#include <sys/time.h>  

#define SIZE	(2)

int global_index_data[2]={0x24,0x45};
pthread_t threads[3];

void* (*function_ptr[3])(void);

void* read_data(void)
{
	const int* ptr = global_index_data;
	//*ptr+=1;	//should fail
	static int i=0;
	for(i=0;i<SIZE;i+=1)
		printf("Data Read:0x%x\n",ptr[i]);
	return NULL;
}

void* thread_1(void)
{
	int* const ptr = &global_index_data[0];
	*ptr = 0xFF;
	printf("Thread-1 modified data\n");
	return NULL;
}

void* thread_2(void)
{
	int* const ptr = &global_index_data[1];
	*ptr = 0xAA;
	printf("Thread-2 modified data\n");
	return NULL;

}


int main(void)
{
	static int i=0;

	function_ptr[0] = thread_1;
	function_ptr[1] = thread_2;
	function_ptr[2] = read_data;

	for(i=0;i<2;i+=1)
		printf("Data original: 0x%x\n",global_index_data[i]);

	for(i=0;i<3;i+=1)
	{
		pthread_create(&threads,NULL,function_ptr[i],0);
		usleep(10000);
	}

	for(i=0;i<3;i+=1)
	{
		pthread_join(threads[i],NULL);
	}

	return 0;
}