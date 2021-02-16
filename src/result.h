#ifndef  RESULT_H
#define RESULT_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <map>
#include <unordered_map>
#include <iomanip>
#include "common.h"
#include "options.h"
#include "writer.h"
#include "barcodeProcessor.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>

using namespace std;

class Result {
public:
	Result(Options* opt, int threadId, bool paired = true);
	~Result();
	Writer* getWriter() { return mWriter; }
	static Result* merge(vector<Result*>& list);
	void print();
	void dumpDNBs(string& mappedDNBOutFile);
	void setBarcodeProcessor(unordered_map<uint64, Position1>* bpmap);
private:
	void setBarcodeProcessor();
public:
	Options* mOptions;
	bool mPaired;
	long mTotalRead;
	long mFxiedFilterRead;
	long mDupRead;
	long mLowQuaRead;
	long mWithoutPositionReads;
	long overlapReadsWithMis = 0;
	long overlapReadsWithN = 0;
	Writer* mWriter;
	BarcodeProcessor* mBarcodeProcessor;
	int mThreadId;
};

#endif // ! RESULT_H
