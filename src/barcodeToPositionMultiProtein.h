#ifndef BarcodeToPositionMultiProtein_H
#define BarcodeToPositionMultiProtein_H

#include <string>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include "options.h"
#include "barcodePositionMap.h"
#include "mapInsertThread.h"
#include "fixedfilter.h"
#include "writerThread.h"
#include "proteinResult.h"
#include "generateGem.h"

using namespace std;

typedef struct ReadPairPack2 {
	ReadPair** data;
	int count;
}ReadPairPack2;

typedef struct ReadPairRepository2 {
	ReadPairPack2** packBuffer;
	atomic_long readPos;
	atomic_long writePos;
}ReadPairRepository2;

class BarcodeToPositionMultiProtein {
public:
	BarcodeToPositionMultiProtein(Options* opt);
	~BarcodeToPositionMultiProtein();
	bool process();
private:
	void initOutput();
	void closeOutput();
	bool processPairEnd(ReadPairPack2* pack, ProteinResult* result);
	void initPackRepositoey();
	void destroyPackRepository();
	void producePack(ReadPairPack2* pack);
	void consumePack(ProteinResult* result);
	void producerTask();
	void consumerTask(ProteinResult* result);
	void writeTask(WriterThread* config);
	void mapInsertTask(MapInsertThread* config);
	
public:
	Options* mOptions;
	BarcodePositionMap* mbpmap;
	ProteinBarcodeMap* proteinBarcodeMap;
	FixedFilter* fixedFilter;
	//unordered_map<uint64, Position*> misBarcodeMap;

private:
	ReadPairRepository2 mRepo;
	atomic_bool mProduceFinished;
	atomic_int mFinishedThreads;
	std::mutex mOutputMtx;
	std::mutex mInputMutx;
	gzFile mZipFile;
	ofstream* mOutStream;
	MapInsertThread* mapInserter;
	WriterThread* mUnmappedWriter1;
	WriterThread* mUnmappedWriter2;
	vector<string> in1;
	vector<string> in2;
	bool filterFixedSequence = false;
};

#endif // ! BarcodeToPositionMultiProtein_H
