/*************************************************************************************************************
* Code reference taken from : 1. https://linux.die.net/man/3/pthread_mutex_lock
*                             2. https://linux.die.net/man/3/clock_gettime
*                             3. https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
*                             4. http://www.cs.tufts.edu/comp/111/examples/Time/clock_gettime.c
**************************************************************************************************************/

#include<stdio.h> 
#include<string.h> 
#include<pthread.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include <time.h> 
#include <sys/time.h>  

#define CLOCK_SOURCE (CLOCK_REALTIME)                                 // Selects the Clock Source
 
pthread_t pthread_arr[2];
pthread_mutex_t MUTEX; 

struct DATA_STRUCT
{
    float x,y,z;                                                     // -500 to 500m/s^2 
    float yaw,pitch,roll;                                            // y = 0 - 360, p,r = -90 to 90
    struct timespec start,finish;
    long time_stamp_sec, time_stamp_nsec;
};

struct DATA_STRUCT data_struct_write;
struct DATA_STRUCT* data_struct_read;

void* (*function_ptr[2])(void);                                       // Function pointer to store the 2 call-back functions

/*********************************************************************
* @brief: Updates the X,Y and Z values with timestamp using mutex APIs.
*********************************************************************/
void* UpdateTime(void) 
{ 
    while(1)
    {
        pthread_mutex_lock(&MUTEX); 
            srand(time(NULL));
            data_struct_write.x = (rand()%1000);                    // Stores random values. Used %1000 to get a better range than %10.
            data_struct_write.y = (rand()%1000);
            data_struct_write.z = (rand()%1000);
            
            data_struct_write.yaw   = rand()%360;
            data_struct_write.pitch = rand()%180;
            data_struct_write.roll  = rand()%180;
            
            if(data_struct_write.x > 500)
            {
                data_struct_write.x=(1000-data_struct_write.x);
                data_struct_write.x*=-1;
            }

            if(data_struct_write.y > 500)
            {
                data_struct_write.y=(1000-data_struct_write.y);
                data_struct_write.y*=-1;
            }
            if(data_struct_write.z > 500)
            {
                data_struct_write.z=(1000-data_struct_write.z);
                data_struct_write.z*=-1;
            }
                
            if(data_struct_write.pitch > 90)
            {
                data_struct_write.pitch=(180-data_struct_write.pitch);
                data_struct_write.pitch*=-1;
            }

            if(data_struct_write.roll > 90)
            {
                data_struct_write.roll=(180-data_struct_write.roll);
                data_struct_write.roll*=-1;
            }
                
            
            clock_gettime(CLOCK_SOURCE,&data_struct_write.finish);
            
            data_struct_write.time_stamp_sec = data_struct_write.finish.tv_sec - data_struct_write.start.tv_sec;
            data_struct_write.time_stamp_nsec= data_struct_write.finish.tv_nsec - data_struct_write.start.tv_nsec;
            
            if (data_struct_write.start.tv_nsec > data_struct_write.finish.tv_nsec) { 
                --data_struct_write.time_stamp_sec; 
                data_struct_write.time_stamp_nsec += 1000000000; 
            } 
        pthread_mutex_unlock(&MUTEX); 
        sleep(1);
        }
} 

/*********************************************************************
* @brief: Reads and displays the X,Y and Z values.
*********************************************************************/
void* GetTime(void) 
{ 
    while(1)
    {
    pthread_mutex_lock(&MUTEX); 
        data_struct_read = &data_struct_write;                        // Pointer to DATA_STRUCT stores the read structure.
        if(data_struct_read!=NULL)
        {
            printf("X:%.2f\n",data_struct_read->x);
            printf("Y:%.2f\n",data_struct_read->y);
            printf("Z:%.2f\n",data_struct_read->z);
            printf("yaw:%.2f\n",data_struct_read->yaw);
            printf("Pitch:%.2f\n",data_struct_read->pitch);
            printf("Roll:%.2f\n",data_struct_read->roll);
            printf("Time stamp:%d.%ds\n",data_struct_read->time_stamp_sec,data_struct_read->time_stamp_nsec);
        }
        printf("Data Read\n");
    pthread_mutex_unlock(&MUTEX); 
    sleep(2);
    }
} 
  
int main(void) 
{ 
    int i = 0; 
    int error; 

    function_ptr[0]=UpdateTime;
    function_ptr[1]=GetTime;
    
    srand(time(NULL));                                                // Initializes random time
    
    if (pthread_mutex_init(&MUTEX, NULL)) 
        printf("Mutex initialization failed\n");
    
    clock_gettime(CLOCK_SOURCE, &data_struct_write.start);
    
    for(i=0;i<2;i+=1)
    {
         pthread_create(&pthread_arr,NULL,function_ptr[i],0);
         sleep(2);
    }
   
    for(i=0;i<2;i+=1)
    {
          pthread_join(pthread_arr[i],0);
    }
    
    pthread_mutex_destroy(&MUTEX); 
  
    return 0; 
} 