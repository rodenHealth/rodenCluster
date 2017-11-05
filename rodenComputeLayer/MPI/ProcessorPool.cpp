
#include <mpi.h>
#include <iostream>
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
#include <boost/serialization/string.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/allocator.hpp>
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
#include "../FirebaseLib/firebase.h"
#include "../ThreadPool/ThreadPool.h"
#include "../ThreadPool/concurrentqueue.h"

string getSubscriptionKey(int rank, int size);

using namespace std;

moodycamel::ConcurrentQueue<FrameData *> *buildQueue(int start, int finish, int &frameCount)
{
  moodycamel::ConcurrentQueue<FrameData *> *partitionQueue = new moodycamel::ConcurrentQueue<FrameData *>();

  // Setup
  FrameData *newFrame;
  frameCount = 0;

  // Load frames
  int frameSize;

  // Push image URLS to queue (this will be paths once we refactor)
  for (int i = start; i < finish; i++)
  {
    char frame[150];
    frameCount++;
    frameSize = sprintf(frame, "https://github.com/rodenHealth/rodenCluster/blob/features/videoProcessing/rodenComputeLayer/VideoProcessor/tmp/frame_%d.jpg?raw=true", i);

    newFrame = new FrameData;
    newFrame->path = frame;
    newFrame->id = i;

    partitionQueue->enqueue(newFrame);
  }

  return partitionQueue;
}

int getStart(int rank, int size, int totalFramesToProcess)
{
  int block = totalFramesToProcess / size;
  int offset = rank * block;
  return offset + 4;
}

int getEnd(int rank, int size, int totalFramesToProcess)
{
  int block = totalFramesToProcess / size;
  int offset = rank * block + block;
  return offset + 4;
}

int main(int argc, char *argv[])
{
  // Setup MPI
  boost::mpi::environment env{argc, argv};
  boost::mpi::communicator world;

  // TODO: This needs to come from somewhere
  int totalFramesToProcess = 60;

  // Get metadata
  int rank = world.rank();
  int size = world.size();

  if (rank == 0) // Manager, build record, tell workers to start
  {
    string videoID;
    FirebaseLib *fb = new FirebaseLib(videoID);

    for (int i = 1; i < size; i++)
    {
      world.send(i, 0, videoID);
    }
  }
  else // Worker
  {
    string vidID;
    world.recv(0, 0, vidID);
    string subKey = getSubscriptionKey(rank, size);
    int start = getStart(rank, size - 1, totalFramesToProcess);
    int finish = getEnd(rank, size - 1, totalFramesToProcess);
    int frameCount;

    moodycamel::ConcurrentQueue<FrameData *> *driverQueue = buildQueue(start, finish, frameCount);

    processQueue(driverQueue, frameCount, subKey, vidID);
  }

  return 0;
}

string getSubscriptionKey(int rank, int size)
{
  int disperse = rank % size;

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