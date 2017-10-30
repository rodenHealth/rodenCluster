/*
    APIThreads

    This is a driver for multithreading concurrent API calls

    As it's configured, it will create 10 threads that will all send
    an API call, wait 1 second and then repeat, until there are no 
    more records in the locked queue
*/

#include <iostream>
#include <unistd.h>
#include <mutex>
#include "APIThreads.h"

using namespace std;

// Knobs
#define NUMTHREADS 10

// Private methods
void incrementCallsMade();
void setBarrier();

// Shared memory
int callsMade = 0;
mutex printMutex;
mutex callsMadeMutex;

// Locked queue: this should contain frames
Queue<int> frameQueue;

int main()
{
    // This is a debug step, we're loading 100 "images"
    for (int i = 0; i < 100; i++)
    {
        frameQueue.push(i);
    }

    // Multitheaded processor
    callThreads(sendAPICallWithTimeout, NUMTHREADS);
}

// TODO: Build this
void sendAPICall()
{
}

// This is the handler we're going to send to the thread pool
void sendAPICallWithTimeout(int threadID)
{
    // Get frame
    auto item = frameQueue.pop();

    // Check frame
    while (item != -1)
    {

        sendAPICall();         // TODO
        print(threadID, item); // Debug

        // Synchronize
        setBarrier();
        wait();
        if (threadID == 0) // Manager new line
        {
            cout << endl;
        }
        // Get next one
        item = frameQueue.pop();
    }
}

// Utility method to synchronize threads
void setBarrier()
{
    incrementCallsMade();
    while (callsMade % NUMTHREADS != 0)
    {
    }
}

// Utility method to update calls made for iteration
void incrementCallsMade()
{
    printMutex.lock();
    callsMade++;

    if (callsMade == NUMTHREADS)
    {
        callsMade = 0;
    }
    printMutex.unlock();
}

// Utility method to clean print thread ID
void print(int thread, int item)
{
    printMutex.lock();
    cout << "Thread [" << thread << "] sending API for image #" << item << endl;
    printMutex.unlock();
}

// Utility method to sleep (we can't flood the API)
void wait()
{
    cout.flush();
    sleep(1.5);
}