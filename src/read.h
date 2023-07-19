#ifndef READ_H
#define READ_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include "sequence.h"
#include "common.h"
#include <vector>

using namespace std;

class Read{
public:
	Read(string name, string seq, string strand, string quality, bool phred64=false);
    Read(string name, Sequence seq, string strand, string quality, bool phred64=false);
	Read(string name, string seq, string strand);
    Read(string name, Sequence seq, string strand);
    Read(Read &r);
	~Read();
	void print();
    void printFile(ofstream& file);
    Read* reverseComplement();
    string firstIndex();
    string lastIndex();
    // default is Q20
    int lowQualCount(int qual=20);
    int length();
    string toString();
    string toStringWithTag(string tag);
    void resize(int len);
    void convertPhred64To33();
    void trimFront(int len);
    void trimBack(int start);
	void getDNBidx(bool isSeq500, int suffixLen = 0, int indexLen=8);
    void getBarcodeFromName(int barcodeLen);

public:
    static bool test();

private:


public:
	string mName;
	Sequence mSeq;
	string mStrand;
	string mQuality;
	string mBarcode;
    uint16 proteinIndex;
    uint32 x;
    uint32 y;
    uint32 umiInt;
	string SEQ500 = "seq500";
	string DEPSEQT1 = "T1";
	int dnbIdx[3];
	bool mHasQuality;
};

class ReadPair{
public:
    ReadPair(Read* left, Read* right);
    ~ReadPair();

    // merge a pair, without consideration of seq error caused false INDEL
    Read* fastMerge();
public:
    Read* mLeft;
    Read* mRight;

public:
    static bool test();
};

#endif