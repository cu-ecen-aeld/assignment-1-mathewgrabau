#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    int rc;
    struct thread_data *thread_func_args = (struct thread_data *)thread_param;
    /* Just in case */
    if (thread_func_args == NULL)
    {
        ERROR_LOG("Terminating thread, NULL thread_param\n");
        return thread_param;
    }

    // Wait for the specified number of milliseconds
    rc = usleep(thread_func_args->wait_to_obtain_ms * 1000);
    if (rc != 0)
    {
        ERROR_LOG("usleep failed on wait_to_obtain_ms: %d", rc);
        thread_func_args->thread_complete_success = false;
    }
    // wait for the mutex
    rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0)
    {
        ERROR_LOG("pthread_mutex_lock failed: %d", rc);
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }

    rc = usleep(thread_func_args->wait_to_release_ms * 1000);
    if (rc != 0)
    {
        ERROR_LOG("usleep failed on wait_to_release_ms: %d", rc);
        thread_func_args->thread_complete_success = false;
    }
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc == 0)
    {
        thread_func_args->thread_complete_success = true;
    }
    else
    {
        ERROR_LOG("pthread_mutex_unlock failed: %d", rc);
    }


    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int rc;

    struct thread_data * thread_func_args = malloc(sizeof(struct thread_data));
    if (thread_func_args == NULL)
    {
        ERROR_LOG("start_thread - malloc failed");
        return false;
    }
    memset(thread_func_args, 0, sizeof(struct thread_data));
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    
    rc = pthread_create(thread, NULL, threadfunc, thread_func_args);
    if (rc != 0)
    {
        ERROR_LOG("pthread_create failed: %d", rc);
        free(thread_func_args);
        return false;
    }

    return true;
}

