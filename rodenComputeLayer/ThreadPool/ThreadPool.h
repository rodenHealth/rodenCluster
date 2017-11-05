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
#include "concurrentqueue.h"

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

void processQueue(moodycamel::ConcurrentQueue<FrameData *> *q, int size, string subKey, string videoID);
