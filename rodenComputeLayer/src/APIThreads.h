#pragma once

#include "../util/RodenThreadLib.h"
#include "../util/RodenLockedQueue.h"

void sendAPICall();
void print(int thread, int item);
void wait();
void sendAPICallWithTimeout(int threadID);