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

// Shared memory
mutex printMutex;
mutex callsMadeMutex;

// Locked queue: this should contain frames
Queue<int> frameQueue;

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

boost::mutex io_mutex;

void thread_fun(boost::barrier &cur_barier, int current)
{
    int item;
    while (1)
    {
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
        wait(1);
    }
}

int main()
{
    // This is a debug step, we're loading 100 "images"
    for (int i = 0; i < 100; i++)
    {
        frameQueue.push(i);
    }

    // Multitheaded processor
    // callThreads(sendAPICallWithTimeout, NUMTHREADS);

    boost::thread_group threads;

    boost::barrier bar(NUMTHREADS);

    for (int i = 0; i < NUMTHREADS; i++)
    {
        threads.add_thread(new boost::thread(thread_fun, boost::ref(bar), i));
    }

    threads.join_all();
}