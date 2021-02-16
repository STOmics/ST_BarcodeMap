#ifndef BARCODETOPOSITIONMULTI_H
#define BARCODETOPOSITIONMULTI_H

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

typedef struct ReadPairPack {
	ReadPair** data;
	int count;
}ReadPairPack;

typedef struct ReadPairRepository {
	ReadPairPack** packBuffer;
	atomic_long readPos;
	atomic_long writePos;
}ReadPairRepository;

class BarcodeToPositionMulti {
public:
	BarcodeToPositionMulti(Options* opt);
	~BarcodeToPositionMulti();
	bool process();
private:
	void initOutput();
	void closeOutput();
	bool processPairEnd(ReadPairPack* pack, Result* result);
	void initPackRepositoey();
	void destroyPackRepository();
	void producePack(ReadPairPack* pack);
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
	ReadPairRepository mRepo;
	atomic_bool mProduceFinished;
	atomic_int mFinishedThreads;
	std::mutex mOutputMtx;
	std::mutex mInputMutx;
	gzFile mZipFile;
	ofstream* mOutStream;
	WriterThread* mWriter;
	WriterThread* mUnmappedWriter;
	bool filterFixedSequence = false;
};

#endif
