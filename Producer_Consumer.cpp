#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <queue>

#define NUM_THREADS 5
#define RUN_TIME 1
#define BUFFER_SIZE 5

using std::cout;
using std::endl;
using std::rand;

int in = 0;
int out = 0;

int empty = BUFFER_SIZE;
int full = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// for pausing threads until all 10 are created
bool stop = true;
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lockThreads = PTHREAD_COND_INITIALIZER;
pthread_mutex_t threadLock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lockThreads2 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t proThreadLocks[BUFFER_SIZE];
pthread_mutex_t conThreadLocks[BUFFER_SIZE];

pthread_cond_t proCond[BUFFER_SIZE];
pthread_cond_t conCond[BUFFER_SIZE];

std::queue<pthread_cond_t *> prodProcesses;
std::queue<pthread_cond_t *> consProcesses;

int buffer[BUFFER_SIZE];

void *Produce(void *threadId)
{
    int tid = *((int *)threadId);

    //wait for pthread_cond_broadcast() in main
    pthread_mutex_lock(&threadLock);
    while (stop)
    {
        pthread_cond_wait(&lockThreads, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);
    // end wait

    while (!stop)
    {
        // semaphore wait for empty
        pthread_mutex_lock(&proThreadLocks[tid - 1]);
        while (empty <= 0)
        {
            prodProcesses.push(&proCond[tid - 1]);
            pthread_cond_wait(&proCond[tid - 1], &proThreadLocks[tid - 1]);
        }
        empty--;
        pthread_mutex_unlock(&proThreadLocks[tid - 1]);
        // end semaphore

        pthread_mutex_lock(&lock);
        // insert
        int inVal = rand() % 10;
        buffer[in] = inVal;
        printf("PRODUCE Thread %d : PRODUCE : %d : in = %d \n", tid, inVal, in);
        in = (in + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&lock);

        // semaphore signal for full
        pthread_mutex_lock(&threadLock);
        full++;
        if (!consProcesses.empty())
        {
            //bug
            pthread_cond_signal(consProcesses.front());
            consProcesses.pop();
        }
        pthread_mutex_unlock(&threadLock);
        // end semaphore
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
        pthread_cond_wait(&lockThreads, &threadLock);
    }
    pthread_mutex_unlock(&threadLock);
    // end wait

    while (!stop)
    {
        // semaphore wait for full
        pthread_mutex_lock(&conThreadLocks[tid - 1]);
        while (full <= 0)
        {
            consProcesses.push(&conCond[tid - 1]);
            pthread_cond_wait(&conCond[tid - 1], &conThreadLocks[tid - 1]);
        }
        full--;
        pthread_mutex_unlock(&conThreadLocks[tid - 1]);
        // end semaphore

        pthread_mutex_lock(&lock);
        //remove
        int outVal = buffer[out];
        printf("CONSUME Thread %d : CONSUME : %d : out = %d \n", tid, outVal, out);
        out = (out + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&lock);

        // semaphore signal for empty
        pthread_mutex_lock(&threadLock2);
        empty++;
        if (!prodProcesses.empty())
        {
            // bug
            pthread_cond_signal(prodProcesses.front());
            prodProcesses.pop();
        }
        pthread_mutex_unlock(&threadLock2);

        // end semaphore
    }

    pthread_exit(NULL);
}

int main(void)
{
    // Create 5 producers and 5 consumers
    pthread_t producers[NUM_THREADS], consumers[NUM_THREADS];
    int statusCreate;
    int labels[NUM_THREADS * 2] = {1, 2, 3, 4, 5}; // used for labelling threads only

    for (int i = 0; i < NUM_THREADS; i++)
    {

        cout << "main() : creating producer thread, " << labels[i] << endl;
        statusCreate = pthread_create(&producers[i], NULL, Produce, (void *)&labels[i]);

        if (statusCreate != 0)
        {
            cout << "Error:unable to create thread," << statusCreate << endl;
            return EXIT_FAILURE;
        }

        cout << "main() : creating consumer thread, " << labels[i] << endl;
        statusCreate = pthread_create(&consumers[i], NULL, Consume, (void *)&labels[i]);

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

    // wait 10 seconds
    sleep(RUN_TIME);
    stop = true;

    // wake sleeping threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_cond_signal(&conCond[i]);
        pthread_cond_signal(&proCond[i]);
    }

    // join threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    // // clean-up
    // pthread_mutex_destroy(&lock);
    // pthread_mutex_destroy(&threadLock);
    // pthread_cond_destroy(&lockThreads);

    cout << endl
         << RUN_TIME << " seconds is up!" << endl;
    cout << "Exiting Gracefully ... " << endl;

    return EXIT_SUCCESS;
}