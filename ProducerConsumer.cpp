#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>

#define NUM_THREADS 5
#define RUN_TIME 10

using std::cout;
using std::endl;
using std::rand;

int numArray[10];
int arrayLen = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//used to stop thread execution until the condition that all threads have been
bool stop = true;
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conditionVar = PTHREAD_COND_INITIALIZER;

void *Produce(void *threadId)
{
    int tid = *((int *)threadId);

    //wait for pthread_cond_broadcast() in main
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&conditionVar, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);
    // end wait

    while (!stop)
    {
        pthread_mutex_lock(&lock);
        if (arrayLen < 10)
        {
            int randNum = rand() % 10;
            numArray[arrayLen] = randNum;
            arrayLen++;
            cout << "Thread " << tid << ": PRODUCED " << randNum << " | array length = " << arrayLen << endl;
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

void *Consume(void *threadId)
{
    int tid = *((int *)threadId);

    //wait for pthread_cond_broadcast() in main
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&conditionVar, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);
    // end wait

    while (!stop)
    {
        pthread_mutex_lock(&lock);
        if (arrayLen != 0)
        {
            arrayLen--;
            cout << "Thread " << tid << ": CONSUMED " << numArray[arrayLen] << " | array length = " << arrayLen << endl;
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

int main(void)
{
    //Create 5 producers and 5 consumers
    pthread_t producers[NUM_THREADS], consumers[NUM_THREADS];
    int statusCreate;
    int labels[NUM_THREADS] = {1, 2, 3, 4, 5}; // used for labelling threads only

    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "main() : creating producer thread, " << labels[i] << endl;
        statusCreate = pthread_create(&producers[i], NULL, Produce, (void *)&labels[i]);
        if (statusCreate)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "main() : creating consumer thread, " << labels[i] << endl;
        statusCreate = pthread_create(&consumers[i], NULL, Consume, (void *)&labels[i]);
        if (statusCreate)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    //signal all threads to start
    pthread_mutex_lock(&threadLock);
    stop = false;
    pthread_cond_broadcast(&conditionVar);
    pthread_mutex_unlock(&threadLock);

    //wait x seconds, then stop thread execution
    sleep(RUN_TIME);
    stop = true;

    //wait for all threads to finish executing
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&threadLock);
    pthread_cond_destroy(&conditionVar);

    cout << endl
         << RUN_TIME << " seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}