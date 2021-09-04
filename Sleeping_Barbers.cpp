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
#define NUM_BARBERS 2
#define CUST_FREQ 3

using std::cout;
using std::endl;
using std::rand;

int waiting[NUM_SEATS];
int waitingLen = 0;
bool stop = false;

// One mutex, condition var, and loop variable for each barber
// Used to put barber to sleep
pthread_mutex_t barberLocks[NUM_BARBERS];
pthread_cond_t barberCondVars[NUM_BARBERS];
bool barberSleep[NUM_BARBERS];

// First-sleep-first-wake to share work (relatively) evenly
std::queue<pthread_cond_t *> barberQueue;
std::queue<int> barberIDQueue;
// Mutex for barberQueue and barberIDQueue
pthread_mutex_t bqMutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex for waitingLen variable
pthread_mutex_t wMutex = PTHREAD_MUTEX_INITIALIZER;

// First-in-first-served for customers
std::queue<int> serviceQueue;

void *Serve(void *threadId)
{
    int tid = *((int *)threadId);
    int totalServed = 0;

    while (!stop)
    {
        bool servedCust = false;
        int custNum = -1;

        // ENTRY SECTION
        pthread_mutex_lock(&wMutex);
        if (waitingLen > 0)
        {
            custNum = serviceQueue.front();
            serviceQueue.pop();
            waitingLen--;
            servedCust = true;
        }
        pthread_mutex_unlock(&wMutex);

        // WORK
        if (servedCust)
        {
            cout << "Barber " << tid << " - serving chair " << custNum << endl;
            // Cut hair
            int cutTime = rand() % 1000 * 1000;
            usleep(cutTime);
            cout << "Barber " << tid << " - finished job " << endl;
            totalServed++;
        }

        // EXIT SECTION
        pthread_mutex_lock(&wMutex);
        if (stop)
        {
            pthread_mutex_unlock(&wMutex);
            // Do not go to sleep after servicing a customer if stop condition is true
            // because other threads will have terminated and will be unable to awake this thread.
        }
        else if (waitingLen == 0)
        {
            pthread_mutex_lock(&bqMutex);
            barberQueue.push(&barberCondVars[tid]);
            barberIDQueue.push(tid);
            pthread_mutex_unlock(&bqMutex);

            cout << "Barber " << tid << " - went to sleep" << endl;
            pthread_mutex_unlock(&wMutex);

            pthread_mutex_lock(&barberLocks[tid]);

            while (barberSleep[tid])
            {

                pthread_cond_wait(&barberCondVars[tid], &barberLocks[tid]);
            }
            barberSleep[tid] = true;
            cout << "Barber " << tid << " - woke up" << endl;
            pthread_mutex_unlock(&barberLocks[tid]);
        }
        else
        {
            pthread_mutex_unlock(&wMutex);
        }
    }

    // Return total customers served
    int *result = (int *)malloc(sizeof(int));
    *result = totalServed;
    pthread_exit(result);
}

void *Enter(void *threadId)
{

    while (!stop)
    {
        pthread_mutex_lock(&wMutex);

        // Check if room full
        if (waitingLen < NUM_SEATS)
        {
            waiting[waitingLen] = waitingLen + 1;
            serviceQueue.push(waitingLen);
            cout << "\nCustomer arrived in chair = " << waitingLen << " Waiting = " << waitingLen + 1 << endl
                 << endl;
            waitingLen++;

            // Wake-up next barber in queue
            pthread_mutex_lock(&bqMutex);
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
            pthread_mutex_unlock(&bqMutex);
        }
        else
        {
            cout << "\nDear customer, all seats are taken, try again later.\n"
                 << endl;
        }
        pthread_mutex_unlock(&wMutex);

        int pauseTime = (rand() % 1000 * 1000) / CUST_FREQ;
        usleep(pauseTime);
    }

    // Wake up any sleeping barbers so program can terminate
    if (stop)
    {
        cout << "\nEnd of shift, all sleeping barbers awake and all working barbers finish up! \n"
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
    pthread_mutex_destroy(&wMutex);
    pthread_mutex_destroy(&bqMutex);
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