#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "queue.h"
#include "http_server.h"

typedef struct {
    pthread_t *threads;
    int thread_count;
    task_queue_t *queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    int stop;
} thread_pool_t;

thread_pool_t* thread_pool_create(int thread_count);
void thread_pool_add_task(thread_pool_t* pool, int client_socket);
void thread_pool_destroy(thread_pool_t* pool);

#endif
