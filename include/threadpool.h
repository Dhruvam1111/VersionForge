#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

/* Task struct: Represents a unit of work */
typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/* Threadpool struct */
typedef struct {
    pthread_mutex_t lock;       // Mutex for synchronization
    pthread_cond_t notify;      // Condition variable to wake up threads
    pthread_t *threads;         // Array of worker threads
    threadpool_task_t *queue;   // Circular buffer for tasks
    int thread_count;           // Number of threads
    int queue_size;             // Max size of the queue
    int head;                   // Queue head index
    int tail;                   // Queue tail index
    int count;                  // Current number of tasks in queue
    int shutdown;               // Flag to signal shutdown
    int started;                // Number of threads started
} threadpool_t;

/**
 * @brief Creates a threadpool with the specified number of threads and queue size.
 */
threadpool_t *threadpool_create(int thread_count, int queue_size);

/**
 * @brief Adds a task to the threadpool.
 * @return 0 on success, -1 on failure.
 */
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument);

/**
 * @brief Destroys the threadpool, waits for all tasks to finish, and cleans up.
 */
int threadpool_destroy(threadpool_t *pool);

#endif // THREADPOOL_H
