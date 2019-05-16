
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>
#include "osqueue.h"

typedef struct thread_pool
{
 int N;                      // the pool size
 pthread_cond_t condition;   // the condition
 pthread_mutex_t mutex;      // the mutex
 OSQueue* taskList;          // the tasks
 OSQueue* pool_threads;      // the threads
 int volatile alive;         // is the pool alive
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
