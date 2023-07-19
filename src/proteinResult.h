#ifndef  PROTEINRESULT_H
#define PROTEINRESULT_H

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
#include "proteinBarcodeProcessor.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>

using namespace std;

class ProteinResult {
public:
	ProteinResult(Options* opt, int threadId, bool paired = true);
	~ProteinResult();
	Writer* getWriter() { return mWriter; }
	static ProteinResult* merge(vector<ProteinResult*>& list);
	void print();
	void dumpDNBs(string& mappedDNBOutFile);
	void setBarcodeProcessor(unordered_map<uint64, Position1>* bpmap, unordered_map<uint64, uint16>* proteinBarcodeMap);
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
	ProteinBarcodeProcessor* mBarcodeProcessor;
	int mThreadId;
};

#endif // ! PROTEINRESULT_H
