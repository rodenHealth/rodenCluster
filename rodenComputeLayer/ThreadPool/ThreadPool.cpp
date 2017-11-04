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
#include "../util/RodenLockedQueue.h"
#include "../util/RodenLockedFrameQueue.h"

using namespace std;

// Knobs
#define NUMTHREADS 10

// Struct
typedef struct FrameData {
    string path;
    int id;
} FrameData;

typedef struct ResultData {
    string result;
    int id;
} ResultData;

// Local methods
void wait(int seconds);
void print(int thread, string item);
void threadHandler(boost::barrier &cur_barier, int current);
bool sendFrame(FrameData* frameData);
size_t callback(const char* in, std::size_t size, std::size_t num, std::string* out);
bool updateFirebase(string data);
string createBaseRecord();
string numToCharSuffix(int num);

// Shared memory
mutex printMutex;
mutex exitMutex;
mutex apiMutex;
mutex callsMadeMutex;

// Locked queue: this should contain frames
Queue<FrameData*> frameQueue;
Queue<ResultData*> resultQueue;

// Shared memory (single-writer)
string videoID;



void updateFrame(int frameID, string frameData)
{
    char updateRequest[300];
    int updateRequestSize = sprintf(updateRequest, "{\"frame%s\": %s}", numToCharSuffix(frameID).c_str(), frameData.c_str());
    
    updateFirebase(updateRequest);
}

// Entry point
int main()
{
    videoID = createBaseRecord();

    // Load frames
    int frameSize;
    int start = 4;
    int finish = 34;

    int totalFrames = 0;

    FrameData* newFrame;
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
    FrameData* item;
    do
    {
        item = NULL;

        // Get frame from queue
        item = frameQueue.pop();

        if (item == NULL)
        {
            // printExit(current);
            return;
        }

        // TODO: API CALL HERE
        sendFrame(item);

        // Wait 1 together
        wait(1);

    } while (item != NULL); // TODO: Condition to check if queue is empty

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
bool sendFrame(FrameData* frameData)
{

    char postField[150];
    int postFieldSize = sprintf(postField, "{'url':'%s'}", frameData->path.c_str());

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
        string returnValue = *httpData;
        returnValue.erase(0, 1);
        returnValue.erase(returnValue.size() - 1);
        boost::replace_all(returnValue, "scores", "emotion");

        updateFrame(frameData->id, returnValue);
    }
}

bool updateFirebase(string data)
{

    char postURL[150];
    int postURLSize = sprintf(postURL, "https://rodenweb.firebaseio.com/videos/%s.json?auth=Yc8tTOqD9uo8Jq4rcT6uXxsGdqlBltpIuvX1wAoB", videoID.c_str());

    apiMutex.lock();
    CURL *hnd = curl_easy_init();
    
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(hnd, CURLOPT_URL, postURL);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "postman-token: 53c637cd-3932-6c31-33db-8bd972c40b86");
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "content-type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, data.c_str());
    
    CURLcode ret = curl_easy_perform(hnd);
    apiMutex.unlock();
}

string createBaseRecord()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    string uuidString = boost::uuids::to_string(uuid);

    CURL *hnd = curl_easy_init();
    
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(hnd, CURLOPT_URL, "https://rodenweb.firebaseio.com/videos.json?auth=Yc8tTOqD9uo8Jq4rcT6uXxsGdqlBltpIuvX1wAoB");
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "postman-token: 53c637cd-3932-6c31-33db-8bd972c40b86");
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "content-type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    

    char postField[150];
    int postFieldSize = sprintf(postField, "{\"%s\": {\"timestamp\":\"%s\"}}", uuidString.c_str(), "1");
    
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, postField);
    
    CURLcode ret = curl_easy_perform(hnd);

    return uuidString;
}

string numToCharSuffix(int num)
{
	string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	string suffix;
	
	int aCount = num / 26;
	int sigDigit = num % 26;
	
	for (int i = 0; i < aCount; i++)
	{
		suffix.append("Z");
	}
	
	suffix += letters[sigDigit];
	
	
	return suffix;
}