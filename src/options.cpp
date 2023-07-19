#include "options.h"

Options::Options()
{
}

void Options::init()
{
	
}

bool Options::validate()
{
	if (in.empty() || out.empty()) {
		cerr << "please give input file and output file" << endl;
		exit(-1);
		//return false;
	}
	if (actionInt == 1) {
		if (transBarcodeToPos.in1.empty() || transBarcodeToPos.in2.empty()) {
			cerr << "please give fastq files";
			exit(-1);
			//return false;
		}
		vector<string> in1;
		vector<string> in2;
		split(transBarcodeToPos.in1, in1);
		split(transBarcodeToPos.in2, in2);
		for (int i=0; i< in1.size(); i++){
			check_file_valid(in1[i]);
			check_file_valid(in2[i]);
		}
	}
	if (! maskFile.empty()){
		check_file_valid(maskFile);
	}

	if (barcodeSegment<=0){
		cerr << "barcodeSegment should >0, but get: " << barcodeSegment << ". set to be the default value 1"<<endl;
		barcodeSegment = 1;
		barcodeStat.segment = 1;
	}
	
	return true;
}

int Options::transRC(string isRC){
	int rc = 0;
	transform(isRC.begin(), isRC.end(), isRC.begin(), towlower);
	if (isRC.compare("false")==0){
		rc = 0;
	}else if (isRC.compare("true")==0){
		rc = 1;
	}else if (isRC.compare("all")==0){
		rc = 2;
	}else{
		loginfo("RC option should one of [true, false, all], but got: " + isRC);
		exit(1);
	}
	return rc;
}

bool Options::getIsSeq500(string& platform){
	if (platform.compare("SEQ500") == 0 || platform.compare("seq500") == 0 || platform.compare("Seq500") == 0 || platform.compare("SEq500") == 0 || platform.compare("SeQ500") == 0){
		return true;
	}else{
		return false;
	}
}

void Options::setFovRange(string fovRange){
	vector<string> ranges;
	split(fovRange, ranges, "_");
	if (ranges.size() < 2){
		loginfo("fov format is wrong");
		exit(1);
	}
	vector<string> colRange;
	vector<string> rowRange;
	split(ranges.at(0), colRange, "-");
	split(ranges.at(1), rowRange, "-");
	if (colRange.size() < 2 || rowRange.size() < 2){
		loginfo("fov format is wrong");
		exit(1);
	}
	drawHeatMap.minCol = stoi(colRange.at(0));
	drawHeatMap.maxCol = stoi(colRange.at(1));
	drawHeatMap.minRow = stoi(rowRange.at(0));
	drawHeatMap.maxRow = stoi(rowRange.at(1));
}
