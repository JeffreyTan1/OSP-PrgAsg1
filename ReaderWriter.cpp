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
 * 
*/

#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>

#define NUM_THREADS 5
#define RUN_TIME 10

using std::cout;
using std::endl;
using std::rand;

int resource = 0;
int readCount = 0;

pthread_mutex_t mutex;
pthread_mutex_t writeMutex;

bool stop = true;
// bool pauseRead = false;
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lockThreads = PTHREAD_COND_INITIALIZER;

void *Read(void *threadId)
{
    int tid = *((int *)threadId);

    // wait for signal to start
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&lockThreads, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);
    // end wait

    do
    {
        // increment readers and lock writers
        pthread_mutex_lock(&mutex);
        readCount++;
        if (readCount == 1)
        {
            pthread_mutex_lock(&writeMutex);
            cout << "Lock Writing" << endl;
        }
        pthread_mutex_unlock(&mutex);

        // reading
        cout << "R-Thread: " << tid << " READ: " << resource << endl;

        // decrement readers and unlock writers
        pthread_mutex_lock(&mutex);
        readCount--;
        if (readCount == 0)
        {
            pthread_mutex_unlock(&writeMutex);
            cout << "Unlock Writing" << endl;
        }
        pthread_mutex_unlock(&mutex);

        sleep(1);

    } while (!stop);

    pthread_exit(NULL);
}

void *Write(void *threadId)
{

    int tid = *((int *)threadId);

    // wait for signal to start
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&lockThreads, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);
    // end wait

    do
    {
        pthread_mutex_lock(&writeMutex);
        resource = rand() % 10;

        // write
        cout << "W-Thread: " << tid << " WRITE: " << resource << endl;

        pthread_mutex_unlock(&writeMutex);
        sleep(1);
    } while (!stop);

    pthread_exit(NULL);
}

int main()
{
    pthread_t writeThreads[NUM_THREADS], readThreads[NUM_THREADS];
    int statusCreate;
    int labels[NUM_THREADS] = {1, 2, 3, 4, 5}; // used for labelling threads only

    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "main() : creating write thread, " << labels[i] << endl;
        statusCreate = pthread_create(&writeThreads[i], NULL, Write, (void *)&labels[i]);
        if (statusCreate != 0)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }

        cout << "main() : creating read thread, " << labels[i] << endl;
        statusCreate = pthread_create(&readThreads[i], NULL, Read, (void *)&labels[i]);
        if (statusCreate != 0)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }
    }

    // signal threads to start
    pthread_mutex_lock(&threadLock);
    stop = false;
    pthread_cond_broadcast(&lockThreads);
    pthread_mutex_unlock(&threadLock);

    // sleep for 10 seconds
    sleep(RUN_TIME);

    stop = true;

    // join threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(readThreads[i], NULL);
        pthread_join(writeThreads[i], NULL);
    }

    cout << endl
         << RUN_TIME << " seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}
