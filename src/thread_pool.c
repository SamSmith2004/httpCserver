#include "thread_pool.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

static void *worker_thread(void *arg);

thread_pool_t *thread_pool_create(int thread_count) {
  thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
  if (pool == NULL) {
    return NULL;
  }

  pool->thread_count = thread_count;
  pool->threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
  if (pool->threads == NULL) {
    free(pool);
    return NULL;
  }

  pool->queue = create_queue();
  if (pool->queue == NULL) {
    free(pool->threads);
    free(pool);
    return NULL;
  }

  // Mutex to protect shared data from being simultaneous accessed by multiple
  // threads
  if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) {
    free(pool->queue);
    free(pool->threads);
    free(pool);
    return NULL;
  }

  // Condition variable to signal threads when there is work to do
  if (pthread_cond_init(&pool->queue_cond, NULL) != 0) {
    pthread_mutex_destroy(&pool->queue_mutex);
    free(pool->queue);
    free(pool->threads);
    free(pool);
    return NULL;
  }

  pool->stop = 0; // Stop flag

  // Create worker threads
  for (int i = 0; i < thread_count; i++) {
    if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
      thread_pool_destroy(pool);
      return NULL;
    }
  }
  return pool;
}

static void *worker_thread(void *arg) {
  thread_pool_t *pool = (thread_pool_t *)arg;
  int client_socket;
  struct timespec timeout_spec;
  struct timeval current_time;

  while (1) {
    pthread_mutex_lock(&pool->queue_mutex);

    gettimeofday(&current_time, NULL);
    timeout_spec.tv_sec = current_time.tv_sec + 1; // 1 second timeout
    timeout_spec.tv_nsec = current_time.tv_usec * 1000;

    // While the queue is empty and stop flag is not set, wait on the condition
    // variable
    while (is_empty(pool->queue) && !pool->stop) {
      int wait_result = pthread_cond_timedwait(
          &pool->queue_cond, &pool->queue_mutex, &timeout_spec);
      if (wait_result == ETIMEDOUT) {
        // Timeout occurred, check stop flag
        if (pool->stop) {
          pthread_mutex_unlock(&pool->queue_mutex);
          return NULL;
        }
        // Reset timeout
        gettimeofday(&current_time, NULL);
        timeout_spec.tv_sec = current_time.tv_sec + 1;
        timeout_spec.tv_nsec = current_time.tv_usec * 1000;
      }
    }

    // If stop flag is set, unlock mutex and break the loop
    if (pool->stop) {
      pthread_mutex_unlock(&pool->queue_mutex);
      break;
    }

    client_socket = dequeue(pool->queue);

    pthread_mutex_unlock(&pool->queue_mutex);

    // Call handle_client() with the dequeued client socket
    handle_client(client_socket);
  }
  return NULL;
}

void thread_pool_add_task(thread_pool_t *pool, int client_socket) {
  if (pool == NULL || pool->queue == NULL) {
    return; // Can't add task to a NULL pool or queue
  }

  pthread_mutex_lock(&pool->queue_mutex);

  enqueue(pool->queue, client_socket);

  pthread_cond_signal(&pool->queue_cond); // Wake up one waiting thread
  pthread_mutex_unlock(&pool->queue_mutex);
}

void thread_pool_destroy(thread_pool_t *pool) {
  if (pool == NULL) {
    return; // Nothing to destroy if pool is NULL
  }
  pool->stop = 1; // Set the stop flag

  // Lock the queue mutex
  pthread_mutex_lock(&pool->queue_mutex);

  // Wake up all waiting threads and Unlock the queue mutex
  pthread_cond_broadcast(&pool->queue_cond);
  pthread_mutex_unlock(&pool->queue_mutex);

  // Wait for all threads to finish
  for (int i = 0; i < pool->thread_count; i++) {
    pthread_join(pool->threads[i], NULL);
  }

  // Destroy the mutex and condition variable
  pthread_mutex_destroy(&pool->queue_mutex);
  pthread_cond_destroy(&pool->queue_cond);

  // Free the threads array and Destroy the threads queue
  free(pool->threads);
  free_queue(pool->queue);

  free(pool);
}
