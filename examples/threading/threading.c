#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    // Sleeps wait_to_obtain_ms number of milliseconds 
    usleep(1000 * thread_func_args->wait_to_obtain_ms);
    // then obtains the mutex
    pthread_mutex_lock(thread_func_args->mutex);
    // then holds for wait_to_release_ms milliseconds
    usleep(1000 * thread_func_args->wait_to_release_ms);
    // then releases
    pthread_mutex_unlock(thread_func_args->mutex);
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
    // Seems the mutex we get is inited already from the test cases
    // The start_thread_obtaining_mutex function should use dynamic memory allocation for thread_data
    // structure passed into the thread.  The number of threads active should be limited only by the
    // amount of available memory.
    struct thread_data *data = malloc(sizeof(struct thread_data));
    if (data == NULL) {
        return false;
    }
    data->thread_complete_success = false;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->mutex = mutex;
    int rc = pthread_create(thread, NULL, threadfunc, data);
    if (rc != 0) {
        free(data);
        return false;
    }

    return true;
}

