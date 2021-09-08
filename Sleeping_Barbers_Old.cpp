#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <queue>

#define RUN_TIME 10
#define NUM_SEATS 4
// The pause time between customer arrivals is = (rand() % 1000 * 1000) / CUST_FREQ;
// I've found that on my machine the following produces an interesting output i.e. at some points all the seats are full
// but there isn't too much spam of customers saying that the seats are full
// NUM_BARBERS = 1, CUST_FREQ = 2
// NUM_BARBERS = 2, CUST_FREQ = 3
// NUM_BARBERS = 3, CUST_FREQ = 4
#define NUM_BARBERS 3
#define CUST_FREQ 3

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

// First-sleep-first-wake to share work (relatively) evenly

// TODO: use tree map to sort by total served?
std::queue<pthread_cond_t *> barberQueue;
std::queue<int> barberIDQueue;

// Mutex for shared variables: waitingLen, waiting[], barberQueue and barberIDQueue
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// First-in-first-served for customers
std::queue<int> serviceQueue;

// Stop execution until all threads created
pthread_mutex_t startThreadsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t startThreadsCond = PTHREAD_COND_INITIALIZER;
bool waitStart = true;

std::string spacer = "                                                                        ";

void *Serve(void *threadId)
{
    int tid = *((int *)threadId);
    int totalServed = 0;

    // wait for signal to start
    pthread_mutex_lock(&startThreadsLock);
    while (waitStart)
    {
        pthread_cond_wait(&startThreadsCond, &startThreadsLock);
    }
    pthread_mutex_unlock(&startThreadsLock);
    // end wait

    while (!stop)
    {
        bool servedCust = false;
        int custNum = -1;

        // entry section
        pthread_mutex_lock(&mutex);

        cout << spacer << "Barber " << tid << " | mutex locked |"
             << "entry section" << endl;

        if (waitingLen > 0)
        {
            custNum = serviceQueue.front();
            serviceQueue.pop();
            waitingLen--;
            servedCust = true;
        }
        pthread_mutex_unlock(&mutex);

        cout << spacer << "Barber " << tid << " | mutex unlocked |"
             << "entry section" << endl;

        // WORK
        if (servedCust)
        {
            cout << "Barber " << tid << " - serving chair "
                 << custNum
                 << endl;

            // Cut hair
            int cutTime = rand() % 1000 * 1000;
            usleep(cutTime);
            cout << "Barber " << tid << " - finished job"
                 << endl;
            totalServed++;
        }

        // exit section
        pthread_mutex_lock(&mutex);

        cout << spacer << "Barber " << tid << " | mutex locked |"
             << "exit section" << endl;

        if (stop)
        {
            pthread_mutex_unlock(&mutex);

            cout << spacer << "Barber " << tid << " | mutex unlocked |"
                 << "exit section" << endl;

            // Do not go to sleep after servicing a customer if stop condition is true
            // because other threads will have terminated and will be unable to awake this thread.
        }
        else if (waitingLen == 0)
        {
            barberQueue.push(&barberCondVars[tid]);
            barberIDQueue.push(tid);

            cout << "Barber " << tid << " - went to sleep" << endl;
            pthread_mutex_unlock(&mutex);

            cout << spacer << "Barber " << tid << " | mutex unlocked |"
                 << "exit section" << endl;

            pthread_mutex_lock(&barberLocks[tid]);
            while (barberSleep[tid])
            {
                pthread_cond_wait(&barberCondVars[tid], &barberLocks[tid]);
            }
            barberSleep[tid] = true;
            cout << "Barber " << tid << " - woke up"
                 << endl;
            pthread_mutex_unlock(&barberLocks[tid]);
        }
        else
        {
            pthread_mutex_unlock(&mutex);

            cout << spacer << "Barber " << tid << " | mutex unlocked |"
                 << "exit section" << endl;
        }
    }

    // Return total customers served
    int *result = (int *)malloc(sizeof(int));
    *result = totalServed;
    pthread_exit(result);
}

void *Enter(void *threadId)
{
    // wait for signal to start
    pthread_mutex_lock(&startThreadsLock);
    while (waitStart)
    {
        pthread_cond_wait(&startThreadsCond, &startThreadsLock);
    }
    pthread_mutex_unlock(&startThreadsLock);
    // end wait

    while (!stop)
    {
        pthread_mutex_lock(&mutex);

        cout << spacer << "Customers | mutex locked" << endl;

        // Check if room full
        if (waitingLen < NUM_SEATS)
        {
            waiting[waitingLen] = waitingLen + 1;
            serviceQueue.push(waitingLen);
            cout << "Customer arrived in chair = " << waitingLen << " | Waiting = " << waitingLen + 1
                 << endl;
            waitingLen++;

            // Wake-up next barber in queue
            if (waitingLen == 1 && !barberQueue.empty() && !barberIDQueue.empty())
            {
                pthread_mutex_lock(&barberLocks[barberIDQueue.front()]);
                int bTid = barberIDQueue.front();
                barberSleep[bTid] = false;
                pthread_cond_signal(barberQueue.front());
                cout << "Signal next barber : "
                     << "\"Wake up barber " << bTid << "!\"" << endl;
                barberIDQueue.pop();
                barberQueue.pop();
                pthread_mutex_unlock(&barberLocks[bTid]);
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
        usleep(pauseTime);
    }

    // Wake up any sleeping barbers so program can terminate
    if (stop)
    {
        cout << "End of shift, all sleeping barbers awake and all working barbers finish up! "
             << endl;
        while (!barberQueue.empty() && !barberIDQueue.empty())
        {
            pthread_mutex_lock(&barberLocks[barberIDQueue.front()]);
            int bTid = barberIDQueue.front();
            barberSleep[bTid] = false;
            pthread_cond_signal(barberQueue.front());
            cout << "Signal next barber : "
                 << "\"Wake up barber " << bTid << "!\"" << endl;
            barberIDQueue.pop();
            barberQueue.pop();
            pthread_mutex_unlock(&barberLocks[bTid]);
        }
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

    // Join all threads
    pthread_join(customers, NULL);

    for (int i = 0; i < NUM_BARBERS; i++)
    {
        void *result;
        pthread_join(barbers[i], &result);
        cout << "Barber " << i << " - total served = " << *((int *)result)
             << endl;
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