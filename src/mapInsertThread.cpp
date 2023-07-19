#include "mapInsertThread.h"

MapInsertThread::MapInsertThread(){
    mInputCounter = 0;
	mOutputCounter = 0;
	mInputCompleted = false;
    mRingBuffer = new uint64*[PACK_NUM_LIMIT];
    
    memset(mRingBuffer, 0, sizeof(uint64*) * PACK_NUM_LIMIT);
	mRingBufferSizes = new size_t[PACK_NUM_LIMIT];
	memset(mRingBufferSizes, 0, sizeof(size_t) * PACK_NUM_LIMIT);
}

MapInsertThread::~MapInsertThread(){
    gemMap.clear();
}

void MapInsertThread::output() {
	if (mOutputCounter >= mInputCounter) {
		usleep(100);
	}
	while (mOutputCounter < mInputCounter) {
		insert(mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]);
		delete mRingBuffer[mOutputCounter];
		mRingBuffer[mOutputCounter] = NULL;
		mOutputCounter++;
		//cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
	}
}

void MapInsertThread::input(uint64* data, size_t size){
    mRingBuffer[mInputCounter] = data;
	mRingBufferSizes[mInputCounter] = size;
	mInputCounter++;
}

void MapInsertThread::insert(uint64* data, size_t size){
    for (int i = 0; i < size; i+=2){
        uint64 key = data[i];
        uint64 umi = data[i+1];
        if (gemMap.count(key) == 0){
            gemMap[key] = {};
			gemMap[key][umi] = 1;
		}
		else if (gemMap[key].count(umi) ==0){
			gemMap[key][umi] = 1;
		}
		else{
        	gemMap[key][umi]++;
        }
    }
}

bool MapInsertThread::isCompleted(){
    return mInputCompleted && (mOutputCounter == mInputCounter);
}

bool MapInsertThread::setInputCompleted(){
    mInputCompleted = true;
	return true;
}

long MapInsertThread::bufferLength() {
	return mInputCounter - mOutputCounter;
}