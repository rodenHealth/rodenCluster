#include <iostream>
#include <unistd.h>
#include <mutex>
#include "APIThreads.h"

using namespace std;

// Private methods
void incrementCallsMade();

// Shared memory
mutex printMutex;
mutex callsMadeMutex;

int callsMade = 0;

int main()
{
    callThreads(sendAPICallWithTimeout, 11);
}

// TODO: Build this
void sendAPICall()
{
}

// This is the handler we're going to send to the thread pool
void sendAPICallWithTimeout(int threadID)
{
    if (threadID == 0) // Manager
    {
    }
    else // Worker
    {
        for (int i = 0; i < 3; i++)
        {
            print(threadID);
            // TODO: Send API
            sendAPICall();
            wait();
        }
    }
}

// Utility method to update calls made for iteration
void incrementCallsMade()
{
    printMutex.lock();
    callsMade++;
    printMutex.unlock();
}

// Utility method to clean print thread ID
void print(int thread)
{
    printMutex.lock();
    cout << "Thread [" << thread << "] sending API call" << endl;
    printMutex.unlock();
}

// Utility method to sleep (we can't flood the API)
void wait()
{
    cout.flush();
    sleep(1.5);
}