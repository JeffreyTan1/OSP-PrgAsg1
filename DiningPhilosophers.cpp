#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>

#define NUM_THREADS 5
#define NUM_FORKS 5
#define RUN_TIME 10

using std::cout;
using std::endl;
using std::rand;

pthread_mutex_t forks[NUM_FORKS];

//used to stop thread execution until the condition that all threads have been
bool stop = true;
pthread_mutex_t threadLock;
pthread_cond_t conditionVar;

void *Philosophise(void *threadId)
{
    int tid = *((int *)threadId) - 1;
    int philosopherNum = *((int *)threadId);
    int firstFork;
    int secondFork;
    int secondForkIndex = tid - 1 < 0 ? NUM_FORKS : tid - 1;

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
        while (!firstFork)
        {
            cout << "Philosopher " << philosopherNum << ": sleeping for first fork - i want " << tid << endl;
            sleep((rand() % 1000) / 1000);
            firstFork = pthread_mutex_trylock(&forks[tid]);
        }

        cout << "Philosopher " << philosopherNum << ": successful in obtaining first fork - " << tid << endl;

        while (!secondFork)
        {
            cout << "Philosopher " << philosopherNum << ": sleeping for second fork - i want " << secondForkIndex << endl;
            sleep((rand() % 1000) / 1000);
            secondFork = pthread_mutex_trylock(&forks[secondForkIndex]);
        }

        cout << "Philosopher " << philosopherNum << ": successful in obtaining second fork - " << secondForkIndex << endl;

        cout << "Philosopher " << philosopherNum << ": eating using forks " << tid << " and " << secondForkIndex << endl;

        sleep((rand() % 1000) / 1000);
        pthread_mutex_unlock(&forks[tid]);
        pthread_mutex_unlock(&forks[secondForkIndex]);
        firstFork = false;
        secondFork = false;
    }

    pthread_exit(NULL);
}

int main(void)
{
    pthread_t philosophers[NUM_THREADS];
    int statusCreate;
    int labels[NUM_THREADS] = {1, 2, 3, 4, 5}; // used for labelling threads only

    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "main() : creating philosopher, " << labels[i] << endl;
        statusCreate = pthread_create(&philosophers[i], NULL, Philosophise, (void *)&labels[i]);
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

    //wait 10 seconds, then stop thread execution
    sleep(10);
    stop = true;

    //wait for all threads to finish executing
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(philosophers[i], NULL);
    }

    cout << endl
         << "10 seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}