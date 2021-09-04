#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>

#define NUM_THREADS 1
#define RUN_TIME 3
#define NUM_SEATS 4

using std::cout;
using std::endl;
using std::rand;

int waiting[NUM_SEATS];
int waitingLen = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//used to stop thread execution until the condition that all threads have been
bool stop = false;
bool sleeping = false;
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lockThreads = PTHREAD_COND_INITIALIZER;

void *Serve(void *threadId)
{
    int tid = *((int *)threadId);

    while (!stop)
    {
        pthread_mutex_lock(&lock);
        if (waitingLen != 0)
        {
            waitingLen--;
            cout << "Barber " << tid << ": Serving " << waiting[waitingLen] << " | customers waiting is now = " << waitingLen << endl;
        }
        pthread_mutex_unlock(&lock);

        //Simulate service, customers can enter during this time
        sleep((rand() % 1000) / 1000);

        pthread_mutex_lock(&lock);
        if (waitingLen == 0)
        {
            sleeping = true;

            //wait for pthread_cond_broadcast() in Enter()
            cout << "\"Goodnight\", says Barber " << tid << endl;
            pthread_mutex_unlock(&lock);

            pthread_mutex_lock(&threadLock);
            while (sleeping)
            {
                pthread_cond_wait(&lockThreads, &threadLock);
            }
            pthread_mutex_unlock(&threadLock);
            // end wait
        }
        else
        {
            pthread_mutex_unlock(&lock);
        }
    }

    pthread_exit(NULL);
}

void *Enter(void *threadId)
{
    while (!stop)
    {
        pthread_mutex_lock(&lock);
        if (waitingLen < NUM_WAITING)
        {
            waiting[waitingLen] = waitingLen + 1;
            waitingLen++;
            cout << "A customer has walked in, number waiting is now = " << waitingLen << endl;

            if (waitingLen == 1)
            {
                //signal all barber threads to start
                pthread_mutex_lock(&threadLock);
                cout << "wake up barbers!" << endl;
                sleeping = false;
                pthread_cond_broadcast(&lockThreads); //pthread_cond_signal for fairness?
                pthread_mutex_unlock(&threadLock);
            }
        }

        pthread_mutex_unlock(&lock);

        sleep((rand() % 1000) / 1000);
    }
    pthread_exit(NULL);
}

int main(void)
{
    //Create x barbers and one customer generator
    pthread_t barbers[NUM_BARBERS], customers;
    int statusCreate;
    int labels[NUM_BARBERS] = {1};

    for (int i = 0; i < NUM_BARBERS; i++)
    {
        cout << "main() : creating barber thread, " << labels[i] << endl;
        statusCreate = pthread_create(&barbers[i], NULL, Serve, (void *)&labels[i]);
        if (statusCreate)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    cout << "main() : creating customers thread" << endl;
    statusCreate = pthread_create(&customers, NULL, Enter, NULL);
    if (statusCreate)
    {
        cout << "Error:unable to create thread," << statusCreate << endl;
        return EXIT_FAILURE;
    }

    //wait x seconds, then stop thread execution
    sleep(RUN_TIME);
    stop = true;

    //wait for all threads to finish executing
    pthread_join(customers, NULL);
    for (int i = 0; i < NUM_BARBERS; i++)
    {
        pthread_join(barbers[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&threadLock);
    pthread_cond_destroy(&lockThreads);

    cout << endl
         << RUN_TIME << " seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}