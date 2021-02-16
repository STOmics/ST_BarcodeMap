#ifndef BARCODETOPOSITIONMULTIPE_H
#define BARCODETOPOSITIONMULTIPE_H

#include <string>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <functional>
#include "options.h"
#include "barcodePositionMap.h"
#include "barcodeProcessor.h"
#include "fixedfilter.h"
#include "writerThread.h"
#include "result.h"

using namespace std;

typedef struct ReadPairPack1 {
	ReadPair** data;
	int count;
}ReadPairPack1;

typedef struct ReadPairRepository1 {
	ReadPairPack1** packBuffer;
	atomic_long readPos;
	atomic_long writePos;
}ReadPairRepository1;

class BarcodeToPositionMultiPE {
public:
	BarcodeToPositionMultiPE(Options* opt);
	~BarcodeToPositionMultiPE();
	bool process();
private:
	void initOutput();
	void closeOutput();
	bool processPairEnd(ReadPairPack1* pack, Result* result);
	void initPackRepositoey();
	void destroyPackRepository();
	void producePack(ReadPairPack1* pack);
	void consumePack(Result* result);
	void producerTask();
	void consumerTask(Result* result);
	void writeTask(WriterThread* config);
	
public:
	Options* mOptions;
	BarcodePositionMap* mbpmap;
	FixedFilter* fixedFilter;
	//unordered_map<uint64, Position*> misBarcodeMap;

private:
	ReadPairRepository1 mRepo;
	atomic_bool mProduceFinished;
	atomic_int mFinishedThreads;
	std::mutex mOutputMtx;
	std::mutex mInputMutx;
	gzFile mZipFile;
	ofstream* mOutStream;
	WriterThread* mWriter1;
	WriterThread* mWriter2;
	WriterThread* mUnmappedWriter1;
	WriterThread* mUnmappedWriter2;
	bool filterFixedSequence = false;
};

#endif
