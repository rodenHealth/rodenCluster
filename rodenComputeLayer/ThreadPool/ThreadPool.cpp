#include "ThreadPool.h"

using namespace std;

// Knobs
#define NUMTHREADS 10
#define SUBCOUNT 10

// Locked queues
moodycamel::ConcurrentQueue<FrameData *> frameQueue;
moodycamel::ConcurrentQueue<PerfData *> perfQueue;

mutex printMutex;
mutex exitMutex;
mutex apiMutex;
mutex errorMutex;
mutex successMutex;
mutex subMutex;

// Firebase lib
FirebaseLib *fb;

// API Key
string subscriptionKey;
string videoID;

// Perf counting
int totalFrames;
int errorCount;
int sendCount;

// Method prototypes
void wait(int seconds);
void print(int thread, string item);
static void threadHandler(boost::barrier &cur_barier, int current);
bool sendFrame(FrameData *frameData, string subscription, PerfData *perf);
static size_t callback(const char *in, std::size_t size, std::size_t num, std::string *out);
void printMessage(int rank, string data);
void printSubscriptionData(int current, string sub);
static string getCurrentDateTime(bool useLocalTime);
void printSend(int current, string sub);
void printExit(int current);
void writePerfToCSV();
void updateError();
void updateSend();
void logger();

void processQueue(moodycamel::ConcurrentQueue<FrameData *> *q, int size, string subKey, string videoID)
{

    // Build firebase
    subscriptionKey = subKey;
    fb = new FirebaseLib(videoID, 0);

    // Build frameQueue
    totalFrames = size;

    // Brute force copy (need a better locked queue)
    for (int i = 0; i < totalFrames; i++)
    {
        FrameData *item;
        bool found = q->try_dequeue(item);

        if (found)
        {
            frameQueue.enqueue(item);
        }
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
    cout << endl
         << "-----------------------------------" << endl;
    logger();
    cout << "Built: " << threadCount << " threads" << endl;
    logger();
    cout << "Processed: " << totalFrames << " frames" << endl;
    logger();
    cout << "Sent: " << sendCount << " requests" << endl;
    logger();
    cout << "Failed: " << errorCount << endl;
    logger();
    cout << "Error rate: " << sendCount / (errorCount + 0.0) << "%" << endl;
    cout << "-----------------------------------" << endl;

    // Perf stuff
    writePerfToCSV();
}

// Entry point
// int main()
// {
//     string subscriptionKey = "";
//     moodycamel::ConcurrentQueue<FrameData *> *driverQueue = new moodycamel::ConcurrentQueue<FrameData *>();

//     // Setup
//     FrameData *newFrame;
//     int frameCount = 0;

//     // Load frames
//     int frameSize;

//     // TEMPORARY CODE
//     int start = 4;
//     int finish = start + NUMTHREADS;
//     // --------------

//     // Push image URLS to queue (this will be paths once we refactor)
//     for (int i = start; i < finish; i++)
//     {
//         char frame[150];
//         frameCount++;
//         frameSize = sprintf(frame, "https://github.com/rodenHealth/rodenCluster/blob/features/videoProcessing/rodenComputeLayer/VideoProcessor/tmp/frame_%d.jpg?raw=true", i);

//         newFrame = new FrameData;
//         newFrame->path = frame;
//         newFrame->id = i;

//         driverQueue->enqueue(newFrame);
//     }

//     processQueue(driverQueue, frameCount, subscriptionKey);

//     return 0;
// }

void writePerfToCSV()
{
    PerfData *perf;
    ofstream perfOutputFile;
    bool validPerfData;

    char fileName[150];
    sprintf(fileName, "bin/%s.csv", getCurrentDateTime(0).c_str());

    char vidID[150];
    sprintf(vidID, "Video ID: %s\n", videoID.c_str());

    perfOutputFile.open(fileName);

    perfOutputFile << "VideoID: " << videoID << "\n";
    perfOutputFile << "Thread ID, Timestamp, API Endpoint (Subscription ID), Success?\n";
    do
    {
        validPerfData = perfQueue.try_dequeue(perf);

        if (validPerfData)
        {
            perfOutputFile << perf->id << "," << perf->timestamp << "," << perf->apiEndpoint.c_str() << "," << perf->success << endl;
        }
    } while (validPerfData);
}

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

    // string subscriptionKey = getSubscriptionKey(current);
    // printSubscriptionData(current, subscriptionKey);

    FrameData *item;
    PerfData *perf;
    bool validFrame;

    do
    {
        // Get frame from queue
        validFrame = frameQueue.try_dequeue(item);

        if (validFrame)
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

    } while (validFrame); // TODO: Condition to check if queue is empty

    return;
}

// Makes the API call for processing
// TODO: This method still needs to send the actual frame instead of bogus data
bool sendFrame(FrameData *frameData, string subscription, PerfData *perf)
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
        frameQueue.enqueue(frameData);
    }

    perfQueue.enqueue(perf);

    return true;
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
    const char *in,
    std::size_t size,
    std::size_t num,
    std::string *out)
{
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

string getCurrentDateTime(bool useLocalTime)
{
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

    char currentTime[84] = "";
    sprintf(currentTime, "%s:%d", buffer, milli);

    return currentTime;
}

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