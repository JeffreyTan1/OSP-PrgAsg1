#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>

#define NUM_THREADS 5

using std::cout;
using std::endl;
using std::rand;

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(0, 10);

int numArray[10];
int arrayLen = 0;
bool stop = true;
pthread_mutex_t lock;
pthread_mutex_t threadLock;
pthread_cond_t conditionVar;

void *Produce(void *threadId)
{
    int tid = *((int *)threadId);

    //wait for broadcast that all threads are created for execution to begin
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&conditionVar, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);

    while (!stop)
    {
        pthread_mutex_lock(&lock);
        if (arrayLen < 10)
        {
            int randNum = distribution(generator);
            numArray[arrayLen] = randNum;
            arrayLen++;
            cout << "Produced, " << randNum << " from thread, " << tid << " array length, " << arrayLen << endl;
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

void *Consume(void *threadId)
{
    int tid = *((int *)threadId);

    //wait for broadcast that all threads are created for execution to begin
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&conditionVar, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);

    while (!stop)
    {
        pthread_mutex_lock(&lock);
        if (arrayLen != 0)
        {
            cout << "Consumed, " << numArray[arrayLen] << " from thread, " << tid << " array length, " << arrayLen - 1 << endl;
            arrayLen--;
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

int main(void)
{
    //Create 5 producers and 5 consumers
    pthread_t producers[NUM_THREADS];
    pthread_t consumers[NUM_THREADS];
    int statusCreate;

    for (int i = 1; i <= NUM_THREADS; i++)
    {
        cout << "main() : creating producer thread, " << i << endl;
        statusCreate = pthread_create(&producers[i - 1], NULL, Produce, (void *)&i);
        if (statusCreate)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            exit(-1);
        }
    }

    for (int i = 1; i <= NUM_THREADS; i++)
    {
        cout << "main() : creating consumer thread, " << i << endl;
        statusCreate = pthread_create(&consumers[i - 1], NULL, Consume, (void *)&i);
        if (statusCreate)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            exit(-1);
        }
    }

    //signal all threads to start
    pthread_mutex_lock(&threadLock);
    stop = false;
    pthread_cond_broadcast(&conditionVar);
    pthread_mutex_unlock(&threadLock);

    //wait 10 seconds, then set stop to true
    sleep(3);
    stop = true;

    //wait for all threads to finish running
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    cout << "10 seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;
}