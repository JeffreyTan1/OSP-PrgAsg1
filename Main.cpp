#ifdef RW

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
    pthread_t writeThreads[NUM_THREADS];
    pthread_t readThreads[NUM_THREADS];
    int statusCreate;
    int labels[NUM_THREADS * 2];

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
    for (int i = 0; i < NUM_THREADS * 2; i++)
    {
        pthread_mutex_destroy(&threadLocks[i]);
        pthread_cond_destroy(&threadCondVars[i]);
    }
    pthread_mutex_destroy(&qMutex);

    cout << endl
         << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}

#elif defined BARBERS

#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <queue>
#include <vector>
#include <tuple>

#define RUN_TIME 10
#define NUM_SEATS 4
// The pause time between customer arrivals is = (rand() % 1000 * 1000) / CUST_FREQ;
// I've found that on my machine the following produces an interesting output i.e. at some points all the seats are full
// but there isn't too much spam of customers saying that the seats are full
// NUM_BARBERS = 1, CUST_FREQ = 2
// NUM_BARBERS = 2, CUST_FREQ = 3
// NUM_BARBERS = 3, CUST_FREQ = 4
#define NUM_BARBERS 3
#define CUST_FREQ 4

using std::cout;
using std::endl;
using std::rand;

// Set this variable for the mutex "unlock"/"lock" outputs
int waiting[NUM_SEATS];
int waitingLen = 0;
bool stop = false;

// One mutex, condition var, and loop variable for each barber
// Used to put barber to sleep
pthread_mutex_t barberLocks[NUM_BARBERS];
pthread_cond_t barberCondVars[NUM_BARBERS];
bool barberSleep[NUM_BARBERS];

// Priority queue to wake up the barber with least customers served.
// Uses QueueItem struct as inputs for priority queue
struct QueueItem
{
    int totalServed;
    pthread_cond_t *conditionVar;
    int tid;

    QueueItem(int totalServed, pthread_cond_t *conditionVar, int tid) : totalServed(totalServed), conditionVar(conditionVar), tid(tid)
    {
    }

    bool operator<(const struct QueueItem &other) const
    {
        return totalServed > other.totalServed;
    }
};
std::priority_queue<QueueItem> barberPQueue;

// Mutex for shared variables: waitingLen, waiting[], barberQueue and barberIDQueue
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// First-in-first-served for customers
std::queue<int> serviceQueue;

// Pause execution until all threads created
pthread_mutex_t startThreadsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t startThreadsCond = PTHREAD_COND_INITIALIZER;
bool waitStart = true;

// Makes output more readable
std::string spacer = "                                                                        ";

// Wakes up the next barber in the priority queue
void SignalBarber()
{

    QueueItem bt = barberPQueue.top();
    int totalServed = bt.totalServed;
    pthread_cond_t *conditionVar = bt.conditionVar;
    int tid = bt.tid;
    pthread_mutex_lock(&barberLocks[tid]);
    barberSleep[tid] = false;
    pthread_cond_signal(&*(conditionVar));
    cout << "Signal next barber : "
         << "\"Wake up barber " << tid << "!\"" << endl;
    barberPQueue.pop();

    pthread_mutex_unlock(&barberLocks[tid]);
}

// Call at the start of threads to pause execution
void WaitToStart()
{
    // wait for signal to start
    pthread_mutex_lock(&startThreadsLock);
    while (waitStart)
    {
        pthread_cond_wait(&startThreadsCond, &startThreadsLock);
    }
    pthread_mutex_unlock(&startThreadsLock);
    // end wait
}

void *Serve(void *threadId)
{
    int tid = *((int *)threadId);
    int totalServed = 0;

    WaitToStart();

    while (!stop)
    {
        int custNum = -1;

        // entry section
        pthread_mutex_lock(&mutex);

        cout << spacer << "Barber " << tid << " | mutex locked |"
             << "entry section" << endl;

        if (waitingLen > 0)
        {
            custNum = serviceQueue.front(); // get the customer index to serve later
            serviceQueue.pop();
            waitingLen--; // decrement
        }
        pthread_mutex_unlock(&mutex);

        cout << spacer << "Barber " << tid << " | mutex unlocked |"
             << "entry section" << endl;

        // WORK
        if (custNum != -1)
        {
            cout << "Barber " << tid << " - serving chair "
                 << custNum
                 << endl;

            int cutTime = rand() % 1000 * 1000;
            usleep(cutTime); // work for 0 to 1000 milliseconds
            cout << "Barber " << tid << " - finished job"
                 << endl;
            totalServed++;
        }

        // exit section
        pthread_mutex_lock(&mutex);

        cout << spacer << "Barber " << tid << " | mutex locked |"
             << "exit section" << endl;

        if (stop || waitingLen > 0) // Dont sleep if stop is true, or there are more customers waiting
        {
            pthread_mutex_unlock(&mutex);

            cout << spacer << "Barber " << tid << " | mutex unlocked |"
                 << "exit section" << endl;
        }
        else
        {
            barberPQueue.push(QueueItem(totalServed, &barberCondVars[tid], tid)); // When there are no customers, put self into the priority queue

            cout << "Barber " << tid << " - went to sleep" << endl;
            pthread_mutex_unlock(&mutex);

            cout << spacer << "Barber " << tid << " | mutex unlocked |"
                 << "exit section" << endl;
            pthread_mutex_lock(&barberLocks[tid]);
            while (barberSleep[tid])
            {
                pthread_cond_wait(&barberCondVars[tid], &barberLocks[tid]); // go to sleep
            }
            barberSleep[tid] = true;
            cout << "Barber " << tid << " - woke up"
                 << endl;
            pthread_mutex_unlock(&barberLocks[tid]);
        }
    }

    // Return total customers served
    int *result = (int *)malloc(sizeof(int)); // freed in main
    *result = totalServed;
    pthread_exit(result);
}

void *Enter(void *threadId)
{
    WaitToStart();

    while (!stop)
    {
        pthread_mutex_lock(&mutex);

        cout << spacer << "Customers | mutex locked" << endl;

        // Check if room full
        if (waitingLen < NUM_SEATS)
        {
            waiting[waitingLen] = waitingLen + 1;
            serviceQueue.push(waitingLen); // put customer's index in queue
            cout << "Customer arrived in chair = " << waitingLen << " | Waiting = " << waitingLen + 1
                 << endl;
            waitingLen++;

            // Wake-up next barber in queue if this is the first customer
            if (waitingLen == 1 && !barberPQueue.empty())
            {
                SignalBarber();
            }
        }
        else
        {
            cout << "All seats are taken, try again later."
                 << endl;
        }
        pthread_mutex_unlock(&mutex);

        cout << spacer << "Customers | mutex unlocked"
             << endl;

        int pauseTime = (rand() % 1000 * 1000) / CUST_FREQ;
        usleep(pauseTime); // sleep for 0 to 1000 milliseconds
    }

    pthread_exit(NULL);
}

int main(void)
{
    // Create x barbers and one customer generator
    pthread_t barbers[NUM_BARBERS], customers;
    int statusCreate;
    int labels[NUM_BARBERS];

    for (int i = 0; i < NUM_BARBERS; i++)
    {
        barberSleep[i] = true;
        labels[i] = i;

        cout << "main() : creating barber thread, " << labels[i] << endl;
        statusCreate = pthread_create(&barbers[i], NULL, Serve, (void *)&labels[i]);
        if (statusCreate != 0)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    cout << "main() : creating customers thread" << endl;
    statusCreate = pthread_create(&customers, NULL, Enter, NULL);
    if (statusCreate != 0)
    {
        cout << "Error:unable to create thread," << statusCreate << endl;
        return EXIT_FAILURE;
    }

    // Signal threads to start
    cout << "Signal all threads to start" << endl;
    pthread_mutex_lock(&startThreadsLock);
    waitStart = false;
    pthread_cond_broadcast(&startThreadsCond);
    pthread_mutex_unlock(&startThreadsLock);

    // Wait 10 seconds
    sleep(RUN_TIME);
    stop = true;

    cout << "All sleeping barbers awake and all working barbers finish up! "
         << endl;
    while (!barberPQueue.empty())
    {
        SignalBarber();
    }

    // Join all threads
    pthread_join(customers, NULL);

    for (int i = 0; i < NUM_BARBERS; i++)
    {
        void *result;
        pthread_join(barbers[i], &result);
        cout << "Barber " << i << " - total served = " << *((int *)result)
             << endl;
        free(result);
    }

    // Clean up
    pthread_mutex_destroy(&mutex);
    for (int i = 0; i < NUM_BARBERS; i++)
    {
        pthread_mutex_destroy(&barberLocks[i]);
        pthread_cond_destroy(&barberCondVars[i]);
    }

    cout << endl
         << RUN_TIME << " seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}

#endif