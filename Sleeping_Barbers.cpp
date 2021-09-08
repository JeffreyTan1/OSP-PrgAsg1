#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <queue>
#include <vector>
#include <tuple>

#define RUN_TIME 10
#define NUM_SEATS 4
#define NUM_BARBERS 3
#define CUST_FREQ 4

using std::cout;
using std::endl;
using std::rand;

// Resources
int waiting[NUM_SEATS];
int waitingLen = 0;

// To stop after 10 seconds
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