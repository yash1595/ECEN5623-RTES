/*************************************************************************************************************
* Code reference taken from : 1. https://linux.die.net/man/3/pthread_mutex_lock
*                             2. https://linux.die.net/man/3/clock_gettime
*                             3. https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
*                             4. http://www.cs.tufts.edu/comp/111/examples/Time/clock_gettime.c
*                             5. http://tuxthink.blogspot.com/2013/01/using-pthreadmutextimedlock-in-linux.html
**************************************************************************************************************/

#include<stdio.h> 
#include<string.h> 
#include<pthread.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include <time.h> 
#include <sys/time.h>  

#define CLOCK_SOURCE (CLOCK_REALTIME)                                           //Source can be set as Real-Time or Monotonic.
#define SLEEP_TIME    (30)                                                       //Time for sleep set to 2s as default can be changed.
#define TIME_OUT      (10)
 
pthread_t pthread_arr[2];                                                       //Array to store pthreads.
pthread_mutex_t MUTEX; 

struct DATA_STRUCT                                                              //Structure to store float values of X,Y,Z and time stamp.
{
    float x,y,z;
    struct timespec start,finish;
    long time_stamp_sec, time_stamp_nsec;
};

struct DATA_STRUCT data_struct_write;                                           //Data structure instance to be written into.
struct DATA_STRUCT* data_struct_read;                                           //Pointer to store structure during read.


void* (*function_ptr[2]) (void);                                                //Array of function pointers.

/*******************************************************************************
* @brief: Mutex is locked first then function populates the write_data_structre 
*         with X,Y,Z and time values. the mutex if freed after these operations.
*         sleep() is used to display functionality of timed_mutex.
*******************************************************************************/
void* UpdateTime(void) 
{ 
while(1)
{
    pthread_mutex_lock(&MUTEX); 
    
        struct timespec wait_for_mutex, current_time;
        clock_gettime(CLOCK_SOURCE,&current_time);
        printf("Mutex acquired at %ld.%ld\n",current_time.tv_sec, current_time.tv_nsec);
        
        data_struct_write.x = (rand()%1000)*0.001;                              // Stores random data in range (0-1)
        data_struct_write.y = (rand()%1000)*0.001;
        data_struct_write.z = (rand()%1000)*0.001;
        
        clock_gettime(CLOCK_SOURCE,&data_struct_write.finish);                  // Stores the final time.
        
        data_struct_write.time_stamp_sec = data_struct_write.finish.tv_sec - data_struct_write.start.tv_sec;
        data_struct_write.time_stamp_nsec= data_struct_write.finish.tv_nsec - data_struct_write.start.tv_nsec;
        
        if (data_struct_write.start.tv_nsec > data_struct_write.finish.tv_nsec) // Used to ensure roll-over of clock is compendated
        { 
            --data_struct_write.time_stamp_sec; 
            data_struct_write.time_stamp_nsec += 1000000000; 
        } 
        printf("Data Written\n");
        sleep(SLEEP_TIME);                                                      // Sleep time to induce timed_unlock_mutex
    
    pthread_mutex_unlock(&MUTEX); 
    sleep(TIME_OUT);
}
    return NULL; 
} 

/*******************************************************************************
* @brief: Continuous loop checks if timeout of 10s has elapsed. If it has,
*         it prints "No new data available at <time>". If it hasn't i.e. it has 
*         acquired the mutex, the read structure is populated.
*******************************************************************************/
void* TimedAccess(void)
{
    int ret;
    struct timespec wait_for_mutex, current_time;

    while(1)
    {
        wait_for_mutex.tv_sec=TIME_OUT;
        wait_for_mutex.tv_nsec=0;
        sleep(TIME_OUT);
        ret=pthread_mutex_timedlock(&MUTEX,&wait_for_mutex);
        if(ret!=0)
        {
            clock_gettime(CLOCK_SOURCE,&current_time);
            printf("No new data available at %ld.%ld\n",current_time.tv_sec,current_time.tv_nsec);
        }
        else
        {
            printf("Unlocked\n");  
            data_struct_read = &data_struct_write;
            if(data_struct_read!=NULL)                                                  // Display data if populated
            {
                printf("X:%.2f\n",data_struct_read->x);
                printf("Y:%.2f\n",data_struct_read->y);
                printf("Z:%.2f\n",data_struct_read->z);
                
                printf("Time stamp:%d.%d\n",data_struct_read->time_stamp_sec,data_struct_read->time_stamp_nsec);
                pthread_mutex_unlock(&MUTEX);
            }
        }
     }
}
  
int main(void) 
{ 
    int i = 0; 
    int error; 
    
    srand(time(NULL));                                                          // Random time generation
    
    data_struct_read =NULL;
  
    clock_gettime(CLOCK_SOURCE, &data_struct_write.start);                      // Gets current time
    
    function_ptr[0]=UpdateTime;                                                 // Function pointers updated
    function_ptr[1]=TimedAccess;

    if (pthread_mutex_init(&MUTEX, NULL))                                       // Initializes the mutex
        printf("Mutex initialization failed\n");
    for(i=0;i<2;i+=1)                                                           // Create the threads
        pthread_create(&pthread_arr[i],NULL,function_ptr[i],0);
    for(i=0;i<2;i+=1)                                                           // Join the threads
        pthread_join(pthread_arr[i],0);
    
    pthread_mutex_destroy(&MUTEX); 
  
    return 0; 
} 