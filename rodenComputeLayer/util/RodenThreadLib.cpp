#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include "RodenThreadLib.h"

// Roden multithreaded lib

using namespace std;

void (*handler)(int id);
void *callHandler(void *threadID);

/* 
callThreads
fn - function pointer to void function with 1 int param
numberOfThreads - the number of threads to build

Spins up threads and passes the handler
*/

void callThreads(void (*fn)(int id), int numberOfThreads)
{
    handler = fn;

    pthread_t threads[numberOfThreads];
    int thread;
    int i;

    for (i = 0; i < numberOfThreads; i++)
    {
        thread = pthread_create(&threads[i], NULL, callHandler, (void *)i);

        if (thread)
        {
            cout << "Error creating thread," << thread << endl;
            exit(1);
        }
    }
    pthread_exit(NULL);
}

void *callHandler(void *threadID)
{
    long id;
    id = (long)threadID;

    handler(id);

    pthread_exit(NULL);
    return NULL;
}
