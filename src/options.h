#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cctype>
#include <algorithm>
#include "util.h"

using namespace std;

class DrawHeatMapOptions {
public:
	DrawHeatMapOptions() {
	}

public:
	//hashmap file of the overlaped barcode list
	string map;
	//mask file path
	string maskFile;
	string fovRange;
	//min fov colimn
	int minCol;
	// max fov column
	int maxCol;
	//min fov row
	int minRow;
	//max fov row
	int maxRow;	
	//wether generate q10 heatmap tiff
	bool getQ10Tiff = false;
	string q10TiffFile;
};

class BarcodeOverlapOptions {
public:
	BarcodeOverlapOptions() {

	}
public:
	// map file contianning the barcode hash map of second sequencing
	string in2;
	int mismatch;
	
	// number of base trimed on the front of sequence
	//int frontTrim = 0;
	// number of base trimed on the tail of sequence
	//int tailTrim = 0;
	// map bucket size
};

class BarcodeStatOptions {
public:
	BarcodeStatOptions(){
	}
public:
	int segment;
	// wheather the barcodes of two sequencing are reverse complement
	string rcString;
	int rc;
	string readidSep;
};

class TransBarcodeToPosOptions {
public:
	TransBarcodeToPosOptions() {

	}
public:
	//first sequencing fastq file or barcode map file
	string in;
	//second sequencing fastq file or barcode map file of read1
	string in1;
	//second sequencing fastq file of read2
	string in2;
	// second sequencing output fastq file of read1
	string out1;
	//second sequencing output fastq file of read2
	string out2;
	//allowed max mismatch
	int mismatch;
	//barcode to position map dump file path
	//string bpMapOutFile;
	//file path for reads with unmapped barcode
	string unmappedOutFile;
	//file path for reads with unmapped barcode of read2
	string unmappedOutFile2;
	//which read contains the umi
	int umiRead;
	//umi start position
	int umiStart;
	//umi length
	int umiLen;
	//which read contians the barcode
	int barcodeRead;
	//mapped dnb list file
	string mappedDNBOutFile;
	//fixed sequence that will be detected in the read1.
	string fixedSequence;
	//fixed sequence file contianing fixed sequence
	string fixedSequenceFile;
	//fixed sequence start position
	int fixedStart;
	//if PEoutput was true, PE reads will be writen to output.
	bool PEout = false;
};

class Options {
public:
	Options();
	void init();
	bool validate();
	int transRC(string isRC);
	bool getIsSeq500(string& platform);
	void setFovRange(string fovRange);
public:
	enum actions { map_barcode_to_slide = 1, merge_barcode_list = 2 } action;
	int actionInt = 1;
	// file name of first sequencing read
	string in;
	// output file name
	string out;
	// report output file path
	string report;
	//compression level for gzip output
	int compression;
	int barcodeLen;
	int barcodeStart;
	int barcodeSegment;
	int turnFovDegree;
	string platform;
	string chipID;
	bool isSeq500;
	string maskFile;
	long mapSize = 100000000;
	bool verbose;
	int thread;
	string rcString;
	int rc;
	DrawHeatMapOptions drawHeatMap;
	BarcodeOverlapOptions barcodeOverlap;
	BarcodeStatOptions barcodeStat;
	TransBarcodeToPosOptions transBarcodeToPos;
};

#endif
