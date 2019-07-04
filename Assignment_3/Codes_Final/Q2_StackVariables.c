#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

pthread_t pthread_1, pthread_2;

void* callback_pthread_1(void)
{
    int variable_1=5;
    printf("Variable-1 is : %d\n",variable_1);
    return NULL;
}

void* callback_pthread_2(void)
{
   int variable_1=1;
   printf("Variable-1 is : %d\n",variable_1);
   return NULL;
}

int main(void)
{
    int i = 0;
    int err;


        if(pthread_create(&pthread_1, NULL, &callback_pthread_1, NULL))printf("Thread-1 not created\n");
        if(pthread_create(&pthread_2, NULL, &callback_pthread_2, NULL))printf("Thread-2 not created\n");
        
        pthread_join(pthread_1,NULL);
        pthread_join(pthread_2,NULL);
    
    return 0;
}