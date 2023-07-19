#ifndef MAP_INSERT_THREAD_H
#define MAP_INSERT_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include "common.h"
#include "util.h"
#include <atomic>
#include <mutex>

using namespace std;

class MapInsertThread{
public:
    MapInsertThread();
    ~MapInsertThread();
    void cleanup();

	bool isCompleted();
	void output();
	void input(uint64* data, size_t size);
	bool setInputCompleted();
    void insert(uint64* data, size_t size);

	long bufferLength();

public:
    std::unordered_map<uint64, std::unordered_map<uint64, uint32>> gemMap;
private:
    mutex mtx;
    bool mInputCompleted;
    atomic_long mInputCounter;
	atomic_long mOutputCounter;
    uint64** mRingBuffer;
    size_t* mRingBufferSizes;
};

#endif // ! MAP_INSERT_THREAD_H