# POSIX_Thread_Pool
+ <u><b>support the functions</b></u>:
  * ThreadPool* tpCreate(int numOfThreads): create a treadpool with size of numOfThreads
  * void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks): destroy the treadpool (cleen the resources)
  * int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param): insert a new tast to the threadpool with computeFunc and its params
