#pragma once
/* 
 * RodenThreadLib.h -- public interface to the RodenThreadLib framework
 */

// Takes a void function that gives you int (thread ID)
void callThreads(void (*fn)(int id), int numberOfThreads);
