#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "threadpool.h"

// The worker thread function (Consumer)
static void *threadpool_thread(void *threadpool) {
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    while (1) {
        // 1. Lock the mutex to access the queue safely
        pthread_mutex_lock(&(pool->lock));

        // 2. Wait while the queue is empty AND we are not shutting down
        while ((pool->count == 0) && (!pool->shutdown)) {
            // pthread_cond_wait automatically unlocks mutex and waits for signal
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        // 3. Check if we are shutting down
        if ((pool->shutdown) && (pool->count == 0)) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // 4. Grab a task from the queue
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count--;

        // 5. Unlock the mutex
        pthread_mutex_unlock(&(pool->lock));

        // 6. Execute the task (Work happens here!)
        (*(task.function))(task.argument);
    }

    return NULL;
}

threadpool_t *threadpool_create(int thread_count, int queue_size) {
    threadpool_t *pool;
    int i;

    if (thread_count <= 0 || thread_count > 64 || queue_size <= 0 || queue_size > 65535) {
        return NULL;
    }

    // Allocate memory for the pool struct
    if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
        return NULL;
    }

    // Initialize simple fields
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = 0;
    pool->started = 0;

    // Allocate memory for threads and queue
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

    if ((pool->threads == NULL) || (pool->queue == NULL)) {
        // Cleanup if malloc failed
        if (pool->threads) free(pool->threads);
        if (pool->queue) free(pool->queue);
        free(pool);
        return NULL;
    }

    // Initialize Mutex and Condition Variable
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0)) {
        if (pool->threads) free(pool->threads);
        if (pool->queue) free(pool->queue);
        free(pool);
        return NULL;
    }

    // Create the worker threads
    for (i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool) != 0) {
            threadpool_destroy(pool);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument) {
    int err = 0;
    int next_tail;

    if (pool == NULL || function == NULL) {
        return -1;
    }

    // Lock the queue
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    // Calculate next tail position
    next_tail = (pool->tail + 1) % pool->queue_size;

    // Check if queue is full
    if (pool->count == pool->queue_size) {
        err = -1;
    } 
    // Check if we are shutting down
    else if (pool->shutdown) {
        err = -1;
    } 
    else {
        // Add task to the queue
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next_tail;
        pool->count++;

        // Signal a waiting thread that work is available!
        pthread_cond_signal(&(pool->notify));
    }

    pthread_mutex_unlock(&(pool->lock));

    return err;
}

int threadpool_destroy(threadpool_t *pool) {
    int i, err = 0;

    if (pool == NULL) {
        return -1;
    }

    // Lock
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    // Set shutdown flag
    // This tells threads: "Finish what you are doing, then exit."
    if (pool->shutdown) {
        err = -1; // Already shutting down
    }
    pool->shutdown = 1;

    // Broadcast to wake up ALL threads so they can see the shutdown flag
    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
        err = -1;
    }

    // Wait for threads to finish (Join)
    for (i = 0; i < pool->thread_count; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            err = -1;
        }
    }

    // Clean up resources
    threadpool_task_t *old_queue = pool->queue; // Keep pointer to free later
    pool->queue = NULL; // Prevent use-after-free
    
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    
    free(pool->threads);
    free(old_queue);
    free(pool);

    return err;
}
