#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/bind.hpp>
#include <boost/atomic.hpp>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include "../util/RodenLockedQueue.h"

using namespace std;

// Knobs
#define NUMTHREADS 10

// Local methods
void wait(int seconds);
void print(int thread, int item);
void threadHandler(boost::barrier &cur_barier, int current);

// Shared memory
mutex printMutex;
mutex callsMadeMutex;

// Locked queue: this should contain frames
Queue<int> frameQueue;

// Entry point
int main()
{
    // This is a debug step, we're loading 100 "images"
    for (int i = 0; i < 100; i++)
    {
        frameQueue.push(i);
    }

    // Build thread pool
    boost::thread_group threads;

    // Build synchronization barrier
    boost::barrier bar(NUMTHREADS);

    // Build threads, passing ref to barrier and ID
    for (int i = 0; i < NUMTHREADS; i++)
    {
        threads.add_thread(new boost::thread(threadHandler, boost::ref(bar), i));
    }

    // Wait for threads to complete
    threads.join_all();
}

// Main handler for thread
void threadHandler(boost::barrier &cur_barier, int current)
{
    int item;
    do
    {
        // Get frame from queue
        item = frameQueue.pop();

        // TODO: API CALL HERE
        print(current, item);

        // Sync
        cur_barier.wait();

        // Formatting for output
        if (current == 0) // Manager
        {
            cout << endl;
        }

        // Sync back up
        cur_barier.wait();
        // boost::lock_guard<boost::mutex> locker(io_mutex);

        // Wait 1 together
        wait(1);

        // Sync back up
        cur_barier.wait();

    } while (item != -1); // TODO: Condition to check if queue is empty
}

// Utility method to make a thread sleep
void wait(int seconds)
{
    boost::this_thread::sleep_for(boost::chrono::seconds(seconds));
}

// Utility method to clean print thread ID
void print(int thread, int item)
{
    printMutex.lock();
    cout << "Thread [" << thread << "] sending API for image #" << item << endl;
    printMutex.unlock();
}
