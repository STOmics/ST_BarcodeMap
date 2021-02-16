#ifndef  BARCODE_POSITION_CONFIG_H
#define BARCODE_POSITION_CONFIG_H

#include <stdio.h>
#include <unordered_map>
#include <unordered_set>
#include "common.h"
#include "options.h"
#include "barcodePositionMap.h"

using namespace std;

class BarcodePositionConfig {
public:
	BarcodePositionConfig(Options* opt, int threadId);
	~BarcodePositionConfig();
	static BarcodePositionConfig* merge(BarcodePositionConfig** configs);
	void addBarcode(uint64 barcodeInt, Position1& pos);
	void print();
public:
	Options* mOptions;
	long totalReads;
	long lowQReads;
	long dupReads;
	long withoutPositionRead;
	int mThreadId;
	unordered_set<uint64> barcodeSet;
	unordered_map<uint64, Position1> bpmap;
};

#endif 
