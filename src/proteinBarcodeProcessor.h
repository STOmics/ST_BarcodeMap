#ifndef  PROTIENBARCODEPROCESSOR_H
#define PROTEINBARCODEPROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include "read.h"
#include "util.h"
#include "barcodePositionMap.h"
#include "proteinBarcodeMap.h"
#include "options.h"
#include "util.h"

using namespace std;

class ProteinBarcodeProcessor{
public:
	ProteinBarcodeProcessor(Options* opt, unordered_map<uint64, Position1>* mbpmap, unordered_map<uint64, uint16>* mproteinBarcodeMap);
	ProteinBarcodeProcessor();
	~ProteinBarcodeProcessor();
	bool process(Read* read1, Read* read2);
	void dumpDNBmap(string& dnbMapFile);
private:
	
	void  getUMI(Read* r, pair<string, string>& umi, bool isRead2=false);
	bool getProteinIndex(Read* r, int bStart=0, int bLen=15);
	void decodePosition(const uint32 codePos, pair<uint16, uint16>& decodePos);
	void decodePosition(const uint64 codePos, pair<uint32, uint32>& decodePos);
	uint32 encodePosition(int fovCol, int fovRow);
	uint64 encodePosition(uint32 x, uint32 y);
	long getBarcodeTypes();
	Position1* getPosition(uint64 barcodeInt);
	Position1* getPosition(string& barcodeString);
	uint16 getProteinIndex(uint64 proteinBarcode);
	void misMaskGenerate();
	string positionToString(Position1* position);
	string positionToString(Position* position);
	unordered_map<uint64, Position1>::iterator getMisOverlap(uint64 barcodeInt);
	Position1* getNOverlap(string& barcodeString, uint8 Nindex);
	int getNindex(string& barcodeString);
	bool barcodeStatAndFilter(pair<string, string>& barcode);
	bool barcodeStatAndFilter(string& barcodeQ);
	bool umiStatAndFilter (pair<string, string>& umi);
private:
	uint64* misMask;
	int misMaskLen;
	int* misMaskLens;
	const char q10 = '+';
	const char q20 = '5';
	const char q30 = '?';
	string polyT = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
	uint64 polyTInt;
public:
	Options* mOptions;
	unordered_map<uint64, Position1>* bpmap;
	unordered_map<uint64, uint16>* proteinBarcodeMap;
	long totalReads = 0;
	long mMapToSlideRead = 0;
	long overlapReads = 0;
	long overlapReadsWithMis = 0;
	long overlapReadsWithN = 0;
	long barcodeQ10 = 0;
	long barcodeQ20 = 0;
	long barcodeQ30 = 0;
	long umiQ10 = 0;
	long umiQ20 = 0;
	long umiQ30 = 0;
	long seqQ10 = 0;
	long seqQ20 = 0;
	long seqQ30 = 0;
	long readsWithAdapter = 0;
	long readWithDnb = 0;
	long umiQ10FilterReads = 0;
	long umiNFilterReads = 0;
	long umiPloyAFilterReads = 0;
	long readsMatchProtein = 0;
	int mismatch;
	int barcodeLen;
};


#endif // ! PROTEINBARCODEPROCESSOR_H
