/** 
 * ##   README  ##
 * 
 * Problem: Starvation Problem
 * Writers may only write when the number of readers is zero.
 * 
 * After a reader thread completes a loop, it is instantly ready to read again in the following loop.
 * Suppose we have all 5 reader threads reading simultaneously, everytime the read loop is complete, 
 * the readCount in decremented to 4, only to be incremented back to 5 when the loop is run again.
 * This would make for a very uninteresting output, where the majority would be the read threads reading the 
 * resource with the write threads being unable to execute.
 * 
 * My Solution:
 * I have added sleep(1) to every read and write thread at the end of their loops.
 * This allows us to simulate a more interesting output, where there are a varying number of readers and writers
 * available at any point in time. If the reader is not automatically put into ready mode, the writers have a chance
 * to write to the resource.
 * 
*/

#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <queue>

#define NUM_THREADS 5
#define RUN_TIME 10

using std::cout;
using std::endl;
using std::rand;

int resource = 0;
int readCount = 0;

//rename rmutex
pthread_mutex_t mutex;
pthread_mutex_t wMutex;

bool stop = true;
pthread_mutex_t startThreadsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t startThreadsCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t threadLocks[NUM_THREADS * 2];
pthread_cond_t threadCondVars[NUM_THREADS * 2];
bool threadSleep[NUM_THREADS * 2];

pthread_mutex_t qMutex;
std::queue<pthread_cond_t *> threadQueue;
std::queue<int> threadIDQueue;

std::string spacer = "                                                                        ";

void WaitToStart()
{
    // wait for signal to start
    pthread_mutex_lock(&startThreadsLock);
    while (stop)
    {
        pthread_cond_wait(&startThreadsCond, &startThreadsLock);
    }
    pthread_mutex_unlock(&startThreadsLock);
    // end wait
}
void WaitInQueue(int tid)
{
    pthread_mutex_lock(&qMutex);
    threadQueue.push(&threadCondVars[tid]); // add condition variable to queue
    threadIDQueue.push(tid);                // add threadID to queue
    std::string threadType = tid < NUM_THREADS ? "Write" : "Read";
    cout << threadType << " thread " << tid % NUM_THREADS << " - went to sleep" << endl;
    // cout << "Queue Size = " << threadQueue.size() << endl;
    pthread_mutex_unlock(&qMutex);
    pthread_mutex_lock(&threadLocks[tid]);
    while (threadSleep[tid])
    {
        pthread_cond_wait(&threadCondVars[tid], &threadLocks[tid]); // wait for condition variable signal
    }
    threadSleep[tid] = true;
    cout << threadType << " thread " << tid % NUM_THREADS << " - woke up"
         << endl;
    pthread_mutex_unlock(&threadLocks[tid]);
}
void SignalQueue()
{
    pthread_mutex_lock(&qMutex);
    if (!threadIDQueue.empty())
    {
        pthread_mutex_lock(&threadLocks[threadIDQueue.front()]);
        int tempTid = threadIDQueue.front();
        threadSleep[tempTid] = false;
        pthread_cond_signal(threadQueue.front()); // signal next thread in queue
        std::string threadType = tempTid < NUM_THREADS ? "Write" : "Read";
        cout << "Signal next thread : "
             << "\"Wake up thread TYPE = " << threadType << " ID = "
             << tempTid % NUM_THREADS << "\"" << endl;
        threadIDQueue.pop();
        threadQueue.pop();
        pthread_mutex_unlock(&threadLocks[tempTid]);
        // cout << "Queue Size = " << threadQueue.size() << endl;
    }
    pthread_mutex_unlock(&qMutex);
}

void *Read(void *threadId)
{
    int tid = *((int *)threadId);
    WaitToStart();
    while (!stop)
    {
        WaitInQueue(tid);

        if (!stop)
        {
            // lock readcount mutex
            pthread_mutex_lock(&mutex);
            readCount++;
            if (readCount == 1)
            {
                // lock write mutex
                pthread_mutex_lock(&wMutex);
                cout << spacer << "Lock Writing" << endl;
            }

            // queue signal next
            SignalQueue();

            // release readcount mutex
            pthread_mutex_unlock(&mutex);

            // do reading
            cout << "Read-Thread " << tid % NUM_THREADS << " -- READ: " << resource << endl;

            // lock readcount mutex;
            pthread_mutex_lock(&mutex);
            readCount--;
            if (readCount == 0)
            {
                // release write mutex
                pthread_mutex_unlock(&wMutex);
                cout << spacer << "Unlock Writing" << endl;
            }
            // release readcount mutex;
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(NULL);
}

void *Write(void *threadId)
{

    int tid = *((int *)threadId);

    WaitToStart();

    while (!stop)
    {
        // queue wait
        WaitInQueue(tid);

        if (!stop)
        {
            // lock write mutex
            pthread_mutex_lock(&wMutex);
            cout << spacer << "Lock Writing" << endl;
            // queue signal next
            SignalQueue();

            // do writing
            resource = rand() % 10;
            cout << "W-Thread: " << tid << " WRITE: " << resource << endl;

            // release write mutex
            pthread_mutex_unlock(&wMutex);
            cout << spacer << "Unlock Writing" << endl;
        }
    }

    pthread_exit(NULL);
}

int main()
{
    pthread_t writeThreads[NUM_THREADS], readThreads[NUM_THREADS];
    int statusCreate;
    int labels[NUM_THREADS];

    // Create Write threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        threadSleep[i] = true;
        labels[i] = i;
        cout << "main() : creating write thread, " << labels[i] << endl;
        statusCreate = pthread_create(&writeThreads[i], NULL, Write, (void *)&labels[i]);
        if (statusCreate != 0)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    // Create Read threads
    for (int i = NUM_THREADS; i < NUM_THREADS * 2; i++)
    {
        threadSleep[i] = true;
        labels[i] = i;
        cout << "main() : creating read thread, " << labels[i] % NUM_THREADS << endl;
        statusCreate = pthread_create(&readThreads[i], NULL, Read, (void *)&labels[i]);
        if (statusCreate != 0)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    // signal all threads to start
    pthread_mutex_lock(&startThreadsLock);
    stop = false;
    pthread_cond_broadcast(&startThreadsCond);
    pthread_mutex_unlock(&startThreadsLock);

    // wait for threads to enter queue
    cout << "\n**********Waiting a second for all threads to enter queue**********\n"
         << endl;

    bool initialising = true;
    while (initialising)
    {
        pthread_mutex_lock(&qMutex);
        if (threadQueue.size() == NUM_THREADS * 2)
        {
            initialising = false;
        }
        pthread_mutex_unlock(&qMutex);
    }

    cout << "\n**********Queue full, lets begin!**********\n"
         << endl;

    // start first in queue
    SignalQueue();

    // sleep for 10 seconds
    sleep(RUN_TIME);
    stop = true;

    cout << endl
         << "****************" << RUN_TIME << " seconds is up! "
         << "All sleeping threads awake and all working threads finish up! "
         << "****************" << endl;

    pthread_mutex_lock(&qMutex);
    while (!threadQueue.empty() && !threadIDQueue.empty())
    {
        pthread_mutex_lock(&threadLocks[threadIDQueue.front()]);
        int bTid = threadIDQueue.front();
        threadSleep[bTid] = false;
        pthread_cond_signal(threadQueue.front());
        cout << "Signal next thread : "
             << "\"Wake up thread " << bTid << "!\"" << endl;
        threadIDQueue.pop();
        threadQueue.pop();
        pthread_mutex_unlock(&threadLocks[bTid]);
        cout << "Queue Size = " << threadQueue.size() << endl;
    }
    pthread_mutex_unlock(&qMutex);

    // pthread_t dummy[1];
    // pthread_create(&dummy[0], NULL, Dummy, (void *)&labels[0]);

    // pthread_join(dummy[0], NULL);

    // cout << "joined feelsgoodman" << endl;

    // join threads
    // for (int i = 0; i < NUM_THREADS; i++)
    // {
    //     cout << "join iteration " << i << endl;
    //     // pthread_join(writeThreads[i], NULL);
    //     // pthread_join(readThreads[i], NULL);
    // }

    cout << "**********Wait queue to empty***********" << endl;
    bool emptying = true;
    while (emptying)
    {
        pthread_mutex_lock(&qMutex);
        if (threadQueue.size() == 0)
        {
            emptying = false;
        }
        pthread_mutex_unlock(&qMutex);
    }
    cout << "**********Queue empty***********" << endl;

    cout << "**********Join****************" << endl;

    // cout << pthread_join(writeThreads[0], NULL) << endl;
    // cout << pthread_join(writeThreads[1], NULL) << endl;
    // cout << pthread_join(writeThreads[2], NULL) << endl;
    // cout << pthread_join(writeThreads[3], NULL) << endl;
    // cout << pthread_join(writeThreads[4], NULL) << endl;

    // cout << pthread_join(readThreads[0], NULL) << endl;
    // cout << pthread_join(readThreads[1], NULL) << endl;
    // cout << pthread_join(readThreads[2], NULL) << endl;
    // cout << pthread_join(readThreads[3], NULL) << endl;
    // cout << pthread_join(readThreads[4], NULL) << endl;

    // clean
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&wMutex);
    pthread_mutex_destroy(&startThreadsLock);
    pthread_cond_destroy(&startThreadsCond);
    for (int i = 0; i < NUM_THREADS * 2; i++)
    {
        pthread_mutex_destroy(&threadLocks[i]);
        pthread_cond_destroy(&threadCondVars[i]);
    }
    pthread_mutex_destroy(&qMutex);

    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}
