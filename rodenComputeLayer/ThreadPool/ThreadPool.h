#pragma once

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
#include "../util/RodenLockedQueue.h"

using namespace std;

// Structs
typedef struct FrameData
{
    string path;
    int id;
} FrameData;

typedef struct PerfData
{
    string apiEndpoint;
    string timestamp;
    int id;
    bool success;
} PerfData;

class ThreadPool
{
  private:
    mutex printMutex;
    mutex exitMutex;
    mutex apiMutex;
    mutex errorMutex;
    mutex successMutex;
    mutex subMutex;

    FirebaseLib *fb;
    Queue<FrameData *> frameQueue;
    Queue<PerfData *> perfQueue;

    int totalFrames;

    // Perf counting
    int errorCount;
    int sendCount;

    void wait(int seconds);
    void print(int thread, string item);
    static void threadHandler(boost::barrier &cur_barier, int current);
    bool sendFrame(FrameData *frameData, string subscription, PerfData *perf);
    static size_t callback(const char *in, std::size_t size, std::size_t num, std::string *out);
    void printMessage(int rank, string data);
    void printSubscriptionData(int current, string sub);
    static string getCurrentDateTime(bool useLocalTime);
    static string getSubscriptionKey(int rank);
    void printSend(int current, string sub);
    void printExit(int current);
    void writePerfToCSV();
    void updateError();
    void updateSend();
    void logger();

  public:
    ThreadPool(Queue<FrameData *> q, int size);
    Queue<PerfData *> processQueue();
};