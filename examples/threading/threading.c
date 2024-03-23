#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)
#define MS_TO_US (1000)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    int return_val;
    
    //step 1 wwait to obtain mutex in ms
    return_val = usleep (thread_func_args->wait_to_obtain_ms * MS_TO_US);
    if(return_val != 0)
    {
	ERROR_LOG("obtain mutex usleep failed\n");
	thread_func_args->thread_complete_success = false;
	return thread_param;
    }
    DEBUG_LOG("waiting to obtain mutex successful\n"); 
    
    
    //step 2 obtain mutex lock
    return_val = pthread_mutex_lock(thread_func_args->mutex);
    if(return_val != 0)
    {
	ERROR_LOG("locking mutex failed\n");
	thread_func_args->thread_complete_success = false;
	return thread_param;
    }
    DEBUG_LOG("obtaining mutex successful\n"); 
        
    //step 3 wait to release mutex in ms
    return_val = usleep (thread_func_args->wait_to_release_ms * MS_TO_US);
    if(return_val != 0)
    {
	ERROR_LOG("release mutex usleep failed\n");
	thread_func_args->thread_complete_success = false;
	return thread_param;
    }
    DEBUG_LOG("waiting to release mutex successful\n"); 
        
    //release mutex
    return_val = pthread_mutex_unlock(thread_func_args -> mutex);
    if(return_val != 0)
    {
    	ERROR_LOG("unlocking mutex failed\n");
	thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     
     //step 1 allocate memory for thread_data
     //The start_thread_obtaining_mutex function should use dynamic memory allocation for thread_data
     //structure passed into the thread.  The number of threads active should be limited only by the
     //amount of available memory.
     struct thread_data *thread_data_start_ptr = (struct thread_data*)malloc(sizeof(struct thread_data));
     if (thread_data_start_ptr == NULL)
     {
     	ERROR_LOG("Malloc for thread_data failed \n");
     	return false;
     }
     
     //step 2 setup mutex
     thread_data_start_ptr->mutex = mutex;
     
     //step 3 setup wait args
     thread_data_start_ptr->thread_complete_success = false;
     thread_data_start_ptr->wait_to_obtain_ms = wait_to_obtain_ms;
     thread_data_start_ptr->wait_to_release_ms = wait_to_release_ms;

     //step 4 pass thread_data to created thread * using threadfunc() as entry point.
     //create thread using pthread_create
     
     int rc = pthread_create ( thread,
     				NULL,
     				threadfunc,
     				thread_data_start_ptr);
     if (rc != 0 )
     {
     	ERROR_LOG("Pthread_Create failed\n");
        free(thread_data_start_ptr);
        return false;
     }
     
    DEBUG_LOG("start_thread_obtaining_mutex success\n"); 
    return true;
}

