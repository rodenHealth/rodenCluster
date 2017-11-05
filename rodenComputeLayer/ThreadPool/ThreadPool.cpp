#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/atomic.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <mutex>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <fstream>
#include "../util/RodenLockedQueue.h"
#include "../util/RodenLockedFrameQueue.h"
#include "../FirebaseLib/firebase.h"

using namespace std;

// Knobs
#define NUMTHREADS 50
#define SUBCOUNT 10

// Struct
typedef struct FrameData {
    string path;
    int id;
} FrameData;

typedef struct PerfData {
    string apiEndpoint;
    string timestamp;
    int id;
    bool success;
} PerfData;



// Local methods
void wait(int seconds);
void print(int thread, string item);
void threadHandler(boost::barrier &cur_barier, int current);
bool sendFrame(FrameData* frameData, string subscription, PerfData* perf);
size_t callback(const char* in, std::size_t size, std::size_t num, std::string* out);
void printMessage(int rank, string data);
string getCurrentDateTime(bool useLocalTime);
void writePerfToCSV();


// Shared memory
mutex printMutex;
mutex exitMutex;
mutex apiMutex;
mutex errorMutex;
mutex successMutex;

// Locked queue: this should contain frames
Queue<FrameData*> frameQueue;
Queue<PerfData*> perfQueue;

// Shared memory (single-writer)
FirebaseLib* fb;
bool firstFinisher;

// Perf counting
int errorCount;
int sendCount;

void updateSend()
{
    successMutex.lock();
    sendCount++;
    successMutex.unlock();
}

void updateError()
{
    errorMutex.lock();
    errorCount++;
    errorMutex.unlock();
}

void logger()
{
    cout << "[" << getCurrentDateTime(0) << "]: ";
}


// Entry point
int main()
{
    // Setup
    FrameData* newFrame;
    fb = new FirebaseLib();
    sendCount = 0;
    errorCount = 0;
    int totalFrames = 0;    

    // Load frames
    int frameSize;

    // TEMPORARY CODE
    int start = 4;
    int finish = 35;
    // --------------

    // Push image URLS to queue (this will be paths once we refactor)
    for (int i = start; i < finish; i++)
    {
        char frame[150];
        totalFrames++;
        frameSize = sprintf(frame, "https://github.com/rodenHealth/rodenCluster/blob/features/videoProcessing/rodenComputeLayer/VideoProcessor/tmp/frame_%d.jpg?raw=true", i);
        
        newFrame = new FrameData;
        newFrame->path = frame;
        newFrame->id = i;
        
        frameQueue.push(newFrame);
    }


    // Build thread pool
    boost::thread_group threads;

    // Build synchronization barrier
    boost::barrier bar(NUMTHREADS);

    
    int threadCount = 0;

    logger();
    cout << "Building " << NUMTHREADS << " threads" << endl;

    // Build threads, passing ref to barrier and ID
    for (int i = 0; i < NUMTHREADS; i++)
    {
        threadCount++;
        threads.add_thread(new boost::thread(threadHandler, boost::ref(bar), i));
    }

    // Wait for threads to complete
    threads.join_all();


    // Final stats
    logger();
    if ((sendCount - errorCount) == totalFrames)
    {
        cout << "Successfully processed all " << totalFrames << " frames." << endl;
    }
    else
    {
        cout << "Some error occured and all frames were not processed...." << endl;
    }
    cout << endl << "-----------------------------------" << endl;
    logger();
    cout << "Built: " << threadCount << " threads" << endl;
    logger();
    cout << "Processed: " << totalFrames << " frames" << endl;
    logger();
    cout << "Sent: " << sendCount << " requests" << endl;
    logger();
    cout << "Failed: " << errorCount << endl;
    logger();
    cout << "Error rate: " << sendCount / (errorCount + 0.0) <<  "%" << endl;
    cout << "-----------------------------------" << endl;


    // Perf stuff
    writePerfToCSV();

    return 0;
}

string getSubscriptionKey(int rank)
{
    int disperse = rank % SUBCOUNT;

    switch (disperse)
    {
        case 0:
            return "566d7088f7bc40e2b9afdfc521f957e1";
            break;
        case 1:
            return "ee39b7d84ada4c26859bbb51b391386f";
            break;
        case 2:
            return "c2efe8fdff2244e6afeee461e28a6aa2";
            break;
        case 3:
            return "d1b30e5430a048b1bc3e84c984a2e51e";
            break;
        case 4:
            return "3a40a65192204751b51095bdfa18accc";
            break;
        case 5:
            return "ae99acc5ec754d359c58234614aed1d4";
            break;
        case 6:
            return "01e7e1fab05d4ca8adef35d61f0fd044";
            break;
        case 7:
            return "39824523dc9545758d1d74013d5916fa";
            break;
        case 8:
            return "a4b2f96e978b4416a50ac39ac54a6f18";
            break;
        case 9:
            return "53b3740d68974de98ea2aec7d6829460";
            break;
        default:
            return "";

    }
}

void writePerfToCSV()
{
    PerfData* perf;
    ofstream perfOutputFile;

    char fileName[150];
    sprintf(fileName, "bin/%s.csv", getCurrentDateTime(0).c_str());

    perfOutputFile.open(fileName);

    perfOutputFile << "Thread ID, Timestamp, API Endpoint (Subscription ID), Success?\n";
    do
    {
        perf = perfQueue.pop();

        if (perf != NULL)
        {
            perfOutputFile << perf->id << "," << perf->timestamp << "," <<  perf->apiEndpoint.c_str() << "," << perf->success << endl;
        }
    } while (perf != NULL);
}

mutex subMutex;
void printSubscriptionData(int current, string sub)
{
    subMutex.lock();
    cout << "Thread: " << current << " received: " << sub << endl;
    subMutex.unlock();
}

void printSend(int current, string sub)
{
    subMutex.lock();
    logger();
    cout << "Thread: " << current << " sending to: " << sub << endl;
    subMutex.unlock();
}

// Main handler for thread
void threadHandler(boost::barrier &cur_barier, int current)
{

    string subscriptionKey = getSubscriptionKey(current);
    // printSubscriptionData(current, subscriptionKey);

    FrameData* item;
    PerfData* perf;

    do
    {
        item = NULL;

        // Get frame from queue
        item = frameQueue.pop();

        if (item != NULL)
        {
            // TODO: API CALL HERE

        
            // printSend(current, subscriptionKey);
            perf = new PerfData();
            perf->id = current;
            perf->timestamp = getCurrentDateTime(0);
            perf->apiEndpoint = subscriptionKey;

            sendFrame(item, subscriptionKey, perf);
            
            // We sent one
            updateSend();
        
            // Wait 1
            wait(1);
        }

    } while (item != NULL); // TODO: Condition to check if queue is empty

    return;
}

// Makes the API call for processing
// TODO: This method still needs to send the actual frame instead of bogus data
bool sendFrame(FrameData* frameData, string subscription, PerfData* perf)
{

    char postField[150];
    int postFieldSize = sprintf(postField, "{'url':'%s'}", frameData->path.c_str());

    // curl_global_init(CURL_GLOBAL_ALL);
    // std::string subscriptionKey = "566d7088f7bc40e2b9afdfc521f957e1";
    std::string subscriptionKey = subscription;
    std::string readBuffer;
    CURL *curl = curl_easy_init();
    CURLcode response;
    int httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    // FILE *fd = fopen("obama.jpg", "rb");
    // if (!fd)
    // {
    //     cout << "Cannot find file to send." << endl;
    //     return 1;
    // }

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
        string returnValue = *httpData;
        returnValue.erase(0, 1);
        returnValue.erase(returnValue.size() - 1);
        boost::replace_all(returnValue, "scores", "emotion");

        perf->success = true;

        fb->updateFrame(frameData->id, returnValue);
    }
    else
    {
        perf->success = false;
        updateError();
        frameQueue.push(frameData);
    }

    perfQueue.push(perf);
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

string getCurrentDateTime(bool useLocalTime) {
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;
    
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
    
    char currentTime[84] = "";
    sprintf(currentTime, "%s:%d", buffer, milli);

    return currentTime;
}
