#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/atomic.hpp>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include "../util/RodenLockedQueue.h"

using namespace std;

// Knobs
#define NUMTHREADS 10

// Local methods
void wait(int seconds);
void print(int thread, string item);
void threadHandler(boost::barrier &cur_barier, int current);
bool sendFrame(string imageURL);

// Shared memory
mutex printMutex;
mutex exitMutex;
mutex callsMadeMutex;

// Locked queue: this should contain frames
Queue<string> frameQueue;
Queue<string> resultQueue;

// Entry point
int main()
{
    // Load frames
    char frame[150];
    int frameSize;
    int start = 4;
    int finish = 34;

    int totalFrames = 0;

    for (int i = start; i < finish; i++)
    {
        totalFrames++;
        frameSize = sprintf(frame, "https://github.com/rodenHealth/rodenCluster/blob/features/videoProcessing/rodenComputeLayer/VideoProcessor/tmp/frame_%d.jpg?raw=true", i);
        frameQueue.push(frame);
    }

    // Build thread pool
    boost::thread_group threads;

    // Build synchronization barrier
    boost::barrier bar(NUMTHREADS);

    clock_t begin = clock();

    // Build threads, passing ref to barrier and ID
    for (int i = 0; i < NUMTHREADS; i++)
    {
        threads.add_thread(new boost::thread(threadHandler, boost::ref(bar), i));
    }

    // Wait for threads to complete
    threads.join_all();

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    cout << "Total time to process " << totalFrames << " records: " << elapsed_secs * totalFrames << endl << endl;

    string garbage;
    cout << "Press any key to print results... ";
    cin.get();

    string result;
    while (1)
    {
        result = resultQueue.pop();

        if (result == "")
        {
            break;
        }
        cout << result << endl;
    }

    return 0;
}

void printExit(int current)
{
    printMutex.lock();
    cout << "Thread: " << current << " No more items, leaving..." << endl;
    printMutex.unlock();   
}

void printMessage(int rank, string data)
{
    printMutex.lock();
    cout << "Thread " << rank << " received: " << data << endl;
    printMutex.unlock();   
}

// Main handler for thread
void threadHandler(boost::barrier &cur_barier, int current)
{
    string item;
    do
    {
        item = "";

        // Get frame from queue
        item = frameQueue.pop();

        if (item == "")
        {
            // printExit(current);
            return;
        }
        // printMessage(current, item);

        // TODO: API CALL HERE
        sendFrame(item);

        // Wait 1 together
        wait(1);

    } while (item != ""); // TODO: Condition to check if queue is empty

    return;
}

// Utility method to make a thread sleep
void wait(int seconds)
{
    boost::this_thread::sleep_for(boost::chrono::seconds(seconds));
}

// Utility method to clean print thread ID
void print(int thread, string item)
{
    printMutex.lock();
    cout << "Thread [" << thread << "] sending API for image: " << item << endl;
    printMutex.unlock();
}

size_t callback(
    const char* in,
    std::size_t size,
    std::size_t num,
    std::string* out)
{
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

// Makes the API call for processing
// TODO: This method still needs to send the actual frame instead of bogus data
bool sendFrame(string imageURL)
{

    char postField[150];
    int postFieldSize = sprintf(postField, "{'url':'%s'}", imageURL.c_str());

    curl_global_init(CURL_GLOBAL_ALL);
    std::string subscriptionKey = "566d7088f7bc40e2b9afdfc521f957e1";
    std::string readBuffer;
    CURL *curl = curl_easy_init();
    CURLcode response;
    int httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    FILE *fd = fopen("obama.jpg", "rb");
    if (!fd)
    {
        cout << "Cannot find file to send." << endl;
        return 1;
    }

    // Set required headers and remove unnecesary ones that libcurl automatically sets
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Ocp-Apim-Subscription-Key: 566d7088f7bc40e2b9afdfc521f957e1");
    headers = curl_slist_append(headers, "Transfer-Encoding: chunked");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept:");
    headers = curl_slist_append(headers, "Expect:");

    if (curl)
    {
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_URL, "https://westus.api.cognitive.microsoft.com/emotion/v1.0/recognize");
        curl_easy_setopt(curl, CURLOPT_POST, "1");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField);
    }
    response = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_global_cleanup();

    if (httpCode == 200)
    {
        string *returnValue = httpData.get();
        // cout << *httpData << endl;
        resultQueue.push(*httpData);  }
}

