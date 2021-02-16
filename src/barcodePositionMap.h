#ifndef  BARCODEPOSITIONMAP_H
#define BARCODEPOSITIONMAP_H

#include <string>
#include <cstring>
#include "fastqreader.h"
#include "read.h"
#include "util.h"
#include "options.h"
#include "chipMaskHDF5.h"
#include <unordered_map>
#include <iomanip>
#include <set>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>

using namespace std;

class BarcodePositionMap {
public:
	//BarcodePositionMap(vector<string>& InFile, string MaskFile, int BarcodeStart, int BarcodeLen, int Segment, int TurnFovDegree, bool IsSeq500, int RC, string readidSep = "/");
	//BarcodePositionMap(string InFile, string MaskFile, int BarcodeStart, int BarcodeLen, int Segment, int TurnFovDegree, bool IsSeq500, int RC);
	//BarcodePositionMap(string InFile, int BarcodeStart, int BarcodeLen);
	BarcodePositionMap(Options* opt);
	~BarcodePositionMap();
private:
	void initiate();
	void getSuffixLen();
	int readQualityStat(string& readQ, int index);
	bool barcodeFilter(string& readSeq, int index);
public:
	long getBarcodeTypes();
	void dumpbpmap(string& mapOutFile);
	void loadbpmap();	
	unordered_map<uint64, Position1>* getBpmap() { return &bpmap; };
public:
	unordered_map<uint64,  Position1> bpmap;
	long* totalReads;
	long* readsWithN;
	long* dupReads;
	long* ESTdupReads;
	long* readsWithoutPos;
	long** polyReads;
	long* readsQ10;
	long* readsQ20;
	long* readsQ30;
	long* totalBase;
	long* totalBarcodes;
	int inFastqNumber;
	vector<string> inFile;
	Options* mOptions;
	set<uint64> dupBarcode;
	string maskFile;
	int barcodeStart;
	int barcodeLen;
	int segment;
	int turnFovDegree;
	bool isSeq500;	
	string firstFastq;
	string readidSep;
	int suffixLen = 0;
	int readLen;
	int indexLen = 8;
	//rc==0 only get forward barcode, rc==1 noly get reverse complement barcode, rc==2 get both forward and reverse complement barcode
	int rc;
	slideRange sliderange;
	int slidePitch=500;
	//uint64 polyTint;
};

#endif // ! BARCODEPOSITIONMAP_H