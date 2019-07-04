/*************************************************************************************************************
* Code reference taken from : 1. https:linux.die.net/man/3/pthread_mutex_lock
*                             2. https:linux.die.net/man/3/clock_gettime
*                             3. https:www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
*                             4. http:www.cs.tufts.edu/comp/111/examples/Time/clock_gettime.c
**************************************************************************************************************/

#include<stdio.h> 
#include<string.h> 
#include<pthread.h> 
#include<stdlib.h> 
#include<unistd.h> 
 
#define MUTEX_PRESENT   (0)

pthread_t pthread_arr[2];
pthread_mutex_t MUTEX; 

struct DATA_STRUCT* data_struct_read;

int common_variable, update_status;

void* (*function_ptr[2])(void);
  
void* CallBack1(void) 
{ 
    #if (MUTEX_PRESENT == 1)
    pthread_mutex_lock(&MUTEX);
    #endif 
        sleep(3);
        common_variable+=1;
    #if (MUTEX_PRESENT ==1)
    pthread_mutex_unlock(&MUTEX);
    #endif

    return NULL; 
} 

void* CallBack2(void) 
{ 
    #if (MUTEX_PRESENT == 1)
    pthread_mutex_lock(&MUTEX);
    #endif 
        printf("Updated variable:%d\n",common_variable);
    #if (MUTEX_PRESENT ==1)
    pthread_mutex_unlock(&MUTEX);
    #endif

    return NULL; 
} 
  
int main(void) 
{ 
    int i = 0; 
    int error; 

    function_ptr[0]=CallBack1;
    function_ptr[1]=CallBack2;

    common_variable=0;
    update_status=0;

    
    if (pthread_mutex_init(&MUTEX, NULL)) 
        printf("Mutex initialization failed\n");
    
    for(i=0;i<2;i+=1)
    {
         pthread_create(&pthread_arr,NULL,function_ptr[i],0);
         sleep(1);
    }
   
    for(i=0;i<2;i+=1)
    {
          pthread_join(pthread_arr[i],0);
    }
    
    pthread_mutex_destroy(&MUTEX); 
  
    return 0; 
} 