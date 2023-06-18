//
// Created by Houze ZHANG on 10/06/2023.
//

#include<pthread.h>

// my pc has 8 cores so i choose 7 here, one left for main thread
#define POOL_SIZE 7
// identify the capacity of the queue
#define BUFFER_SIZE 50

struct Task {
    void (*func)(void*);
    void *attr;
};

struct ThreadPool {
    pthread_t threads[POOL_SIZE];

    // at one time, up to POOL_SIZE tasks can be executed for the pool
    struct Task tasks[BUFFER_SIZE];
    size_t queue_size;

    // index of the queue
    size_t queue_front;
    size_t queue_rear;

    // synchronization tools
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;

    // set is_empty to oversee when the process is finish
    pthread_cond_t queue_is_empty;
};

int init_pool(struct ThreadPool *threadPool);
void thread_routine(struct ThreadPool *threadPool);
void assign_task(struct ThreadPool *threadPool, void (*task_func)(void *) , void *task_attr);
void release_pool(struct ThreadPool *threadPool);
