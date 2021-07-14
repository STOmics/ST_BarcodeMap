#include <stdio.h>
#include "fastqreader.h"
#include <time.h>
#include "cmdline.h"
#include <sstream>
#include "util.h"
#include "options.h"
#include "barcodeToPositionMulti.h"
#include "barcodeToPositionMultiPE.h"
#include "barcodeListMerge.h"
#include <mutex>

string command;
mutex logmtx;

int main(int argc, char* argv[]) {
	if (argc == 1) {
		cerr << "spatial_transcriptome: an spatial transcriptome data processor" << endl;
	}

	cmdline::parser cmd;
	// input/output
	cmd.add<string>("in", 'i', "mask file of stereomics chip or input barcode_list file", "");
	cmd.add<string>("in1",  'I', "the second sequencing fastq file path of read1", false, "");
	cmd.add<string>("in2", 0, "the second sequencing fastq file path of read2", false, "");
	cmd.add<string>("barcodeReadsCount", 0, "the mapped barcode list file with reads count per barcode.", false, "");
	cmd.add<string>("out", 'O', "output file prefix or fastq output file of read1", true, "");
	cmd.add<string>("out2", 0, "fastq output file of read2", false, "");
	cmd.add<string>("report", 0, "logging file path.", false, "");
	cmd.add("PEout", 0, "if this option was given, PE reads with barcode tag will be writen");
	cmd.add<int>("compression", 'z', "compression level for gzip output (1 ~ 9). 1 is fastest, 9 is smallest, default is 4.", false, 4);
	cmd.add<string>("unmappedOut", 0, "output file path for barcode unmapped reads of read1, if this path isn't given, discard the reads.", false, "");
	cmd.add<string>("unmappedOut2", 0, "output file path for barcode unmapped reads of read2, if this path isn't given, discard the reads.", false, "");
	cmd.add<uint32_t>("barcodeLen", 'l', "barcode length, default is 25", false, 25);
	cmd.add<int>("barcodeStart", 0, "barcode start position", false, 0);
	cmd.add<int>("umiRead", 0, "read1 or read2 contains the umi sequence.", false, 1);
	cmd.add<int>("barcodeRead", 0, "read1 or read2 contains the barcode sequence.", false, 1);
	cmd.add<int>("umiStart", 0, "umi start position. if the start postion is negative number, no umi sequence will be found", false, 25);
	cmd.add<int>("umiLen", 0, "umi length.", false, 10);
	cmd.add<string>("fixedSequence", 0, "fixed sequence in read1 that will be filtered.", false, "");
	cmd.add<int>("fixedStart", 0, "fixed sequence start position can by specied.", false, -1);
	cmd.add<string>("fixedSequenceFile", 0, "file contianing the fixed sequences and the start position, one sequence per line in the format: TGCCTCTCAG\t-1. when position less than 0, means wouldn't specified", false, "");
	cmd.add<long>("mapSize", 0, "bucket size of the new unordered_map.", false, 0);
	cmd.add<int>("mismatch", 0, "max mismatch is allowed for barcode overlap find.", false, 0);
	cmd.add<int>("action", 0, "chose one action you want to run [map_barcode_to_slide = 1, merge_barcode_list = 2].", false, 1);
	cmd.add<int>("thread", 'w', "number of thread that will be used to run.", false, 2);
	cmd.add("verbose", 'V', "output verbose log information (i.e. when every 1M reads are processed).");

	cmd.parse_check(argc, argv);

	if (argc == 1) {
		cerr << cmd.usage() << endl;
		return 0;
	}

	Options opt;
	opt.in = cmd.get<string>("in");
	opt.out = cmd.get<string>("out");
	opt.compression = cmd.get<int>("compression");
	opt.barcodeLen = cmd.get<uint32_t>("barcodeLen");
	opt.barcodeStart = cmd.get<int>("barcodeStart");
	opt.mapSize = cmd.get<long>("mapSize");
	opt.actionInt = cmd.get<int>("action");
	opt.verbose = cmd.exist("verbose");
	opt.thread = cmd.get<int>("thread");
	opt.rc = opt.transRC(opt.rcString);
	opt.report = cmd.get<string>("report");
	opt.transBarcodeToPos.in = cmd.get < string >("in");
	opt.transBarcodeToPos.in1 = cmd.get<string>("in1");
	opt.transBarcodeToPos.in2 = cmd.get<string>("in2");
	opt.transBarcodeToPos.out1 = cmd.get<string>("out");
	opt.transBarcodeToPos.out2 = cmd.get<string>("out2");
	opt.transBarcodeToPos.mismatch = cmd.get<int>("mismatch");
	opt.transBarcodeToPos.unmappedOutFile = cmd.get<string>("unmappedOut");
	opt.transBarcodeToPos.unmappedOutFile2 = cmd.get<string>("unmappedOut2");
	opt.transBarcodeToPos.umiRead = cmd.get<int>("umiRead");
	opt.transBarcodeToPos.umiStart = cmd.get<int>("umiStart");
	opt.transBarcodeToPos.umiLen = cmd.get<int>("umiLen");
	opt.transBarcodeToPos.barcodeRead = cmd.get<int>("barcodeRead");
	opt.transBarcodeToPos.mappedDNBOutFile = cmd.get<string>("barcodeReadsCount");
	opt.transBarcodeToPos.fixedSequence = cmd.get<string>("fixedSequence");
	opt.transBarcodeToPos.fixedStart = cmd.get<int>("fixedStart");
	opt.transBarcodeToPos.fixedSequenceFile = cmd.get<string>("fixedSequenceFile");
	opt.transBarcodeToPos.PEout = cmd.exist("PEout");
	
	stringstream ss;
	for (int i = 0; i < argc; i++) {
		ss << argv[i] << " ";
	}
	command = ss.str();
	time_t t1 = time(NULL);
	opt.init();
	opt.validate();
	
	if (opt.actionInt == 1) {
		if (opt.transBarcodeToPos.PEout){
			BarcodeToPositionMultiPE barcodeToPosMultiPE(&opt);
			barcodeToPosMultiPE.process();
		}else{
			BarcodeToPositionMulti barcodeToPosMulti(&opt);
			barcodeToPosMulti.process();
		}
	}else if (opt.actionInt == 2) {
		BarcodeListMerge barcodeListMerge(&opt);
		barcodeListMerge.mergeBarcodeLists();
	}
	else {
		cerr << endl << "wrong action has been choosed." << endl;
	}

	time_t t2 = time(NULL);

	cerr << endl << command << endl;
	cerr << "spatialRNADrawMap" << ", time used: " << (t2 - t1) << " seconds" << endl;

	return 0;
}
