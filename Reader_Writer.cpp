#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <queue>

#define NUM_THREADS 5
#define TOTAL_THREADS NUM_THREADS * 2
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

pthread_mutex_t threadLocks[TOTAL_THREADS];
pthread_cond_t threadCondVars[TOTAL_THREADS];
bool threadSleep[TOTAL_THREADS];

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
    cout << threadType << " thread " << tid % NUM_THREADS << " - entered the queue" << endl;
    pthread_mutex_unlock(&qMutex);
    pthread_mutex_lock(&threadLocks[tid]);
    while (threadSleep[tid])
    {
        pthread_cond_wait(&threadCondVars[tid], &threadLocks[tid]); // wait for condition variable signal
    }
    threadSleep[tid] = true;
    cout << threadType << " thread " << tid % NUM_THREADS << " - exited the queue" << endl;
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
             << "\"Wake up thread TYPE = " << threadType << ", ID = "
             << tempTid % NUM_THREADS << "\"" << endl;
        threadIDQueue.pop();
        threadQueue.pop();
        pthread_mutex_unlock(&threadLocks[tempTid]);
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
    int labels[TOTAL_THREADS];

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
    for (int i = NUM_THREADS; i < TOTAL_THREADS; i++)
    {
        threadSleep[i] = true;
        labels[i] = i;
        cout << "main() : creating read thread, " << labels[i] % NUM_THREADS << endl;
        statusCreate = pthread_create(&readThreads[i % NUM_THREADS], NULL, Read, (void *)&labels[i]);
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
    cout << "\n**********Waiting for all threads to enter queue**********\n"
         << endl;

    bool initialising = true;
    while (initialising)
    {
        pthread_mutex_lock(&qMutex);
        if (threadQueue.size() == TOTAL_THREADS)
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

    // Awaken all sleeping threads
    bool cont = true;
    do
    {
        SignalQueue();
        pthread_mutex_lock(&qMutex);
        if (threadQueue.empty() && threadIDQueue.empty())
        {
            cont = false;
        }
        pthread_mutex_unlock(&qMutex);
    } while (cont);

    //join threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(writeThreads[i], NULL);
        pthread_join(readThreads[i], NULL);
    }

    // clean
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&wMutex);
    pthread_mutex_destroy(&startThreadsLock);
    pthread_cond_destroy(&startThreadsCond);
    for (int i = 0; i < TOTAL_THREADS; i++)
    {
        pthread_mutex_destroy(&threadLocks[i]);
        pthread_cond_destroy(&threadCondVars[i]);
    }
    pthread_mutex_destroy(&qMutex);

    cout << endl
         << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}
