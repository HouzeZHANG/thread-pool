//
// Created by Houze ZHANG on 10/06/2023.
//

#include"my_pool.h"
#include<stdio.h>
#include<stdlib.h>

int init_pool(struct ThreadPool *threadPool) {
    threadPool->queue_front = 0;
    threadPool->queue_rear = 0;
    threadPool->queue_size = 0;
    pthread_mutex_init(&threadPool->queue_mutex, NULL);
    pthread_cond_init(&threadPool->queue_not_empty, NULL);
    pthread_cond_init(&threadPool->queue_not_full, NULL);
    pthread_cond_init(&threadPool->queue_is_empty, NULL);

    // let all of the threads ready
    for (size_t i = 0; i < POOL_SIZE; i++) {
        int ret = pthread_create(&threadPool->threads[i],
                                 NULL,
                                 (void *(*)(void *))thread_routine,
                                 threadPool);
        if (ret != 0) {
            return -1;
        }
    }

    return 0;
}

void thread_routine(struct ThreadPool *threadPool) {
    // detach this thread to avoid memory leak
    // pthread_detach(pthread_self());

    while(1) {
        pthread_mutex_lock(&threadPool->queue_mutex);

        // let users assign tasks, waiting until queue is not empty
        while (threadPool->queue_size == 0) {
            pthread_cond_wait(&threadPool->queue_not_empty, &threadPool->queue_mutex);
        }

        // get attributes and task function
        // consume at front
        void (*task_func)(void *) = threadPool->tasks[threadPool->queue_front].func;
        void *task_attr = threadPool->tasks[threadPool->queue_front].attr;
        threadPool->queue_front = (threadPool->queue_front + 1) % BUFFER_SIZE;
        threadPool->queue_size--;

        if (threadPool->queue_size == BUFFER_SIZE - 1) {
            pthread_cond_signal(&threadPool->queue_not_full);
        }
        if (threadPool->queue_size == 0) {
            pthread_cond_signal(&threadPool->queue_is_empty);
        }
        pthread_mutex_unlock(&threadPool->queue_mutex);

        // run the task
        task_func(task_attr);
        free(task_attr);
    }
}

void assign_task(struct ThreadPool *threadPool, void (*task_func)(void *) , void *task_attr) {
    pthread_mutex_lock(&threadPool->queue_mutex);
//    printf("[A] get mutex1\n");

    // producer, wait for not full
    while(threadPool->queue_size == BUFFER_SIZE) {
        pthread_cond_wait(&threadPool->queue_not_full, &threadPool->queue_mutex);
    }
//    printf("[A] get mutex2\n");

    // add at rear
    threadPool->tasks[threadPool->queue_rear].func = task_func;
    threadPool->tasks[threadPool->queue_rear].attr = task_attr;
    threadPool->queue_rear = (threadPool->queue_rear + 1) % BUFFER_SIZE;
    threadPool->queue_size++;

//    printf("[A] rear %d - front %d == size %d ==? %d\n", threadPool->queue_rear,
//           threadPool->queue_front,
//           threadPool->queue_size,
//           (threadPool->queue_rear - threadPool->queue_front) % BUFFER_SIZE == threadPool->queue_size);
    pthread_cond_signal(&threadPool->queue_not_empty);
//    printf("[A] sent signal\n");
    pthread_mutex_unlock(&threadPool->queue_mutex);
//    printf("[A] release mutex\n");
}

void release_pool(struct ThreadPool *threadPool) {
    for (int i = 0; i < POOL_SIZE; i++) {
        pthread_cancel(threadPool->threads[i]);
    }
}