
#include "threadPool.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int initializeMutex(pthread_mutex_t* mutex, int type);
void createThreads(ThreadPool* tp, int numOfThreads);
void* threadPoolFunc(void* param);
void doBroadcast(ThreadPool* threadPool);
void doJoin(ThreadPool* threadPool);
void checkSystemCall(ThreadPool* threadPool, int error);

typedef void (*computeFunction) (void *);  // the task typedef

#define STDERR 2
#define errmsg "Error in system call\n"

/******************
* Function Name: tpCreate
* Input: int (the size of the Thread Pool)
* Output: return the Thread Pool
* Function Operation: create the Thread Pool
******************/
ThreadPool* tpCreate(int numOfThreads) {
    // allocate ThreadPool
    ThreadPool* tp = (ThreadPool*) malloc(sizeof(ThreadPool));
    
    if (tp == NULL) {
        write(STDERR, errmsg, strlen(errmsg));
        exit(-1);
    }
    
    // initialize the fileds
    tp->N = numOfThreads;
    tp->alive = 1;
    tp->taskList = osCreateQueue();
    tp->pool_threads = osCreateQueue();
    
    if (initializeMutex(&tp->mutex, PTHREAD_MUTEX_ERRORCHECK)) {
        osDestroyQueue(tp->taskList);
        osDestroyQueue(tp->pool_threads);

        write(STDERR, errmsg, strlen(errmsg));
        exit(-1);
    }
    
    
    if (pthread_cond_init(&tp->condition, NULL)) {
        pthread_mutex_destroy(&tp->mutex);
        osDestroyQueue(tp->taskList);
        osDestroyQueue(tp->pool_threads);

        write(STDERR, errmsg, strlen(errmsg));
        exit(-1);
    }
    
    createThreads(tp, numOfThreads);
    
    return tp;
}

/******************
* Function Name: tpDestroy
* Input: ThreadPool* (the Thread Pool)
*        int (should Wait For Tasks)
* Function Operation: destroy the Thread Pool
******************/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    if (!threadPool->alive) {
        return;
    }
    threadPool->alive = 0;

    checkSystemCall(threadPool, pthread_mutex_lock(&threadPool->mutex));
    // if not should wait for tasks so we need to empty the queue
    if (!shouldWaitForTasks) {
        while(!osIsQueueEmpty(threadPool->taskList)) {
            osDequeue(threadPool->taskList);
        }
    }

    checkSystemCall(threadPool, pthread_mutex_unlock(&threadPool->mutex));
    
    doJoin(threadPool);
    
    // destroy queue
    osDestroyQueue(threadPool->taskList);
    osDestroyQueue(threadPool->pool_threads);

    // destroy condition
    pthread_cond_destroy(&threadPool->condition);

    // destroy mutex
    pthread_mutex_destroy(&threadPool->mutex);

    // free ThreadPool
    free(threadPool);
}

/******************
* Function Name: tpInsertTask
* Input: ThreadPool* (the Thread Pool)
*        void (*computeFunc) (void *) (the function to run)
*        void* the param of the function
* Function Operation: insert new task to the pool
******************/
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    if (!threadPool->alive) {
        return -1;
    }

    checkSystemCall(threadPool, pthread_mutex_lock(&threadPool->mutex));
    
    osEnqueue(threadPool->taskList, computeFunc);
    osEnqueue(threadPool->taskList, param);
    
    if (osIsQueueEmpty(threadPool->taskList)) {
        pthread_cond_signal(&threadPool->condition);
    }
    checkSystemCall(threadPool, pthread_mutex_unlock(&threadPool->mutex));
    
    return 0;
}

/******************
* Function Name: checkSystemCall
* Input: ThreadPool* (the Thread Pool)
*        int (the return value of the system call
* Function Operation: if there is an error in the system call destroy the pool
******************/
void checkSystemCall(ThreadPool* threadPool, int error) {
    if (error) {
        tpDestroy(threadPool, 0);
        write(STDERR, errmsg, strlen(errmsg));
        exit(-1);
    }
}

/******************
* Function Name: threadPoolFunc
* Input: ThreadPool* (the Thread Pool)
* Function Operation: wait for a new task and run it
******************/
void* threadPoolFunc(void* param) {
    int res;
    computeFunction task;  // the task
    ThreadPool* tp = (ThreadPool*) param;
    while (1) {
        checkSystemCall(tp, pthread_mutex_lock(&tp->mutex));
        res = osIsQueueEmpty(tp->taskList);

        
        if (!tp->alive && res) {
            break;
        } else if (res && tp->alive) {
            checkSystemCall(tp, pthread_cond_wait(&tp->condition, &tp->mutex));
        }

        task = (computeFunction) osDequeue(tp->taskList);
        void* param = osDequeue(tp->taskList);
        
        checkSystemCall(tp, pthread_mutex_unlock(&tp->mutex));
        
        if (task != NULL) {
            task(param);
        }
    }
    checkSystemCall(tp, pthread_mutex_unlock(&tp->mutex));
    return NULL;
}

/******************
* Function Name: createThreads
* Input: ThreadPool* (the Thread Pool)
*        int (the num of threads)
* Function Operation: create and add numOfThreads to the queue
******************/
void createThreads(ThreadPool* tp, int numOfThreads) {
    int i;
    for (i = 0; i < numOfThreads; i++) {
        pthread_t tr;
        checkSystemCall(tp, pthread_create(&tr, NULL, threadPoolFunc, tp));
        osEnqueue(tp->pool_threads, (void*)tr);
    }
}

/******************
* Function Name: doBroadcast
* Input: ThreadPool* (the Thread Pool)
* Function Operation: do broadcast
******************/
void doBroadcast(ThreadPool* threadPool) {
    checkSystemCall(threadPool, pthread_mutex_lock(&threadPool->mutex));
    // do broadcast
    checkSystemCall(threadPool, pthread_cond_broadcast(&threadPool->condition));
    checkSystemCall(threadPool, pthread_mutex_unlock(&threadPool->mutex));
}

/******************
* Function Name: doJoin
* Input: ThreadPool* (the Thread Pool)
* Function Operation: do join to all the threads
******************/
void doJoin(ThreadPool* threadPool) {
    doBroadcast(threadPool);
    // join all threads
    while (!osIsQueueEmpty(threadPool->pool_threads)) {
        
        checkSystemCall(threadPool, pthread_mutex_lock(&threadPool->mutex));
        pthread_t th = (pthread_t)osDequeue(threadPool->pool_threads);
        checkSystemCall(threadPool, pthread_mutex_unlock(&threadPool->mutex));
        
        pthread_join(th, NULL);
    }
}

/******************
* Function Name: initializeMutex
* Input: pthread_mutex_t (the mutex)
*        int (the attribute type)
* Output: return -1 if failed
*         otherwite, return 0
* Function Operation: initialize the mutex with the attr
******************/
int initializeMutex(pthread_mutex_t* mutex, int attr_type){
    pthread_mutexattr_t mutex_attr; // the attr of the mutex
    int res;
    // initialize attr
    res = pthread_mutexattr_init(&mutex_attr);
    if (res) { return -1; }
    
    // set mutex_attr with the attr_type 
    res = pthread_mutexattr_settype(&mutex_attr, attr_type);
    if (res) {
        pthread_mutexattr_destroy(&mutex_attr);
        return -1;
    }
    
    // initialize the mutex with the mutex_attr
    res = pthread_mutex_init(mutex, &mutex_attr);
    if (res) {
        pthread_mutexattr_destroy(&mutex_attr);
        return -1;
    }
    
    // destroy the mutex_attr
    res = pthread_mutexattr_destroy(&mutex_attr);
    if (res) {
        pthread_mutexattr_destroy(&mutex_attr);
        return -1;
    }
    return 0;
}
