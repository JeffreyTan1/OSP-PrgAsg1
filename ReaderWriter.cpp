#include <pthread.h>
#include <iostream>
#include <random>
#include <unistd.h>

#define NUM_THREADS 5
using std::cout;
using std::endl;
using std::rand;

int resource = 0;
pthread_mutex_t mutex;
bool run = true;

void *Read(void *threadId)
{
    int tid = *((int *)threadId);
    while (run)
    {
        cout << "R-Thread: " << tid << " READ: " << resource << endl;
    }
    pthread_exit(NULL);
}

void *Write(void *threadId)
{
    int tid = *((int *)threadId);
    while (run)
    {
        pthread_mutex_lock(&mutex);
        resource = rand() % 10;
        cout << "W-Thread: " << tid << " WRITE: " << resource << endl;
        pthread_mutex_unlock(&mutex);
        }
    pthread_exit(NULL);
}

int main()
{
    pthread_t writeThreads[NUM_THREADS], readThreads[NUM_THREADS];

    int labels[NUM_THREADS] = {1, 2, 3, 4, 5}; // used for labelling threads only

    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "main() : creating write thread, " << labels[i] << endl;
        pthread_create(&writeThreads[i], NULL, Write, (void *)&labels[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "main() : creating read thread, " << labels[i] << endl;
        pthread_create(&readThreads[i], NULL, Read, (void *)&labels[i]);
    }

    sleep(5);
    run = false;

    return EXIT_SUCCESS;
}
