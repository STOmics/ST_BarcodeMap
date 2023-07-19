#include "proteinBarcodeProcessor.h"

ProteinBarcodeProcessor::ProteinBarcodeProcessor(Options* opt, unordered_map<uint64, Position1>* mbpmap, unordered_map<uint64, uint16>* mproteinBarcodeMap)
{
	mOptions = opt;
	bpmap = mbpmap;
	mismatch = opt->transBarcodeToPos.mismatch;
	barcodeLen = opt->barcodeLen;
	polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
	misMaskGenerate();
	proteinBarcodeMap = mproteinBarcodeMap;
}

ProteinBarcodeProcessor::ProteinBarcodeProcessor()
{
}

ProteinBarcodeProcessor::~ProteinBarcodeProcessor()
{

}

bool ProteinBarcodeProcessor::process(Read* read1, Read* read2)
{
	totalReads++;
	string barcode;
	string barcodeQ;
	if (mOptions->transBarcodeToPos.barcodeRead == 1){
		barcode = read1->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
		barcodeQ = read1->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
	}else if (mOptions->transBarcodeToPos.barcodeRead == 2){
		barcode = read2->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
		barcodeQ = read2->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
	}else{
		error_exit("barcodeRead must be 1 or 2 . please check the --barcodeRead option you give");
	}
	barcodeStatAndFilter(barcodeQ);
	Position1* position = getPosition(barcode);

	if (position != nullptr){
		mMapToSlideRead++ ;
		read1->x = position->x;
		read1->y = position->y;
		bool umiPassFilter = true;
		if (mOptions->transBarcodeToPos.umiStart >= 0 && mOptions->transBarcodeToPos.umiLen > 0){
			pair<string, string> umi;
			if (mOptions->transBarcodeToPos.umiRead == 1){
				getUMI(read1, umi);
			}else{
				getUMI(read2, umi, true);
			}
			umiPassFilter = umiStatAndFilter(umi);
		}
		bool proteinMatch = false;
		if (umiPassFilter){
			proteinMatch = getProteinIndex(read2);
		}
		return proteinMatch;
	}
	return false;

}

void ProteinBarcodeProcessor::getUMI(Read* r, pair<string, string>& umi, bool isRead2)
{
	string umiSeq = r->mSeq.mStr.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
	string umiQ = r->mQuality.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
	umi.first = umiSeq;
	umi.second = umiQ;
	uint32 umiInt = seqEncode_1(umiSeq.c_str(), 0, mOptions->transBarcodeToPos.umiLen);
	r->umiInt = umiInt;
	if (isRead2) {
		r->mSeq.mStr = r->mSeq.mStr.substr(0, mOptions->transBarcodeToPos.umiStart);
		r->mQuality = r->mQuality.substr(0, mOptions->transBarcodeToPos.umiStart);
	}
}

bool ProteinBarcodeProcessor::getProteinIndex(Read* r, int bStart, int bLen){
	for (int i = bStart; i<bLen; i++){
		if (r->mQuality[i] >= q30) {
			seqQ30++;
			seqQ20++;
			seqQ10++;
		}
		else if (r->mQuality[i] >= q20) {
			seqQ20++;
			seqQ10++;
		}else if (r->mQuality[i] >= q10) {
			seqQ10++;
		}
	}
	uint64 barcodeInt = seqEncode(r->mSeq.mStr.c_str(), bStart, bLen);
	uint16 proteinIndex = getProteinIndex(barcodeInt);
	r->proteinIndex = proteinIndex;
	if (proteinIndex != 0xFFFF){
		readsMatchProtein++;
		return true;
	}
	return false;
}

void ProteinBarcodeProcessor::decodePosition(const uint32 codePos, pair<uint16, uint16>& decodePos)
{
	decodePos.first = codePos >> 16;
	decodePos.second = codePos & 0x0000FFFF;
}

void ProteinBarcodeProcessor::decodePosition(const uint64 codePos, pair<uint32, uint32>& decodePos)
{
	decodePos.first = codePos >> 32;
	decodePos.second = codePos & 0x00000000FFFFFFFF;
}

uint32 ProteinBarcodeProcessor::encodePosition(int fovCol, int fovRow)
{
	uint32 encodePos = (fovCol << 16) | fovRow;
	return encodePos;
}

uint64 ProteinBarcodeProcessor::encodePosition(uint32 x, uint32 y)
{
	uint64 encodePos = ((uint64)x << 32) | (uint64)y;
	return encodePos;
}

long ProteinBarcodeProcessor::getBarcodeTypes()
{
	return bpmap->size();
}

Position1* ProteinBarcodeProcessor::getPosition(uint64 barcodeInt)
{
	unordered_map<uint64, Position1>::iterator iter = bpmap->find(barcodeInt);
	if (iter!=bpmap->end()) {
		overlapReads++;
		return &iter->second;
	}
	else if (mismatch > 0) {
		iter = getMisOverlap(barcodeInt);
		if (iter != bpmap->end()) {
			overlapReadsWithMis++;
			return &iter->second;
		}
		else {
			return nullptr;
		}
	}
	return nullptr;
}

Position1* ProteinBarcodeProcessor::getPosition(string& barcodeString)
{
	int Nindex = getNindex(barcodeString);
	if (Nindex == -1) {
		uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
		if (barcodeInt == polyTInt) {
			return nullptr;
		}
		return getPosition(barcodeInt);
	}
	else if (Nindex == -2) {
		return nullptr;
	}
	else if (mismatch > 0) {
		return getNOverlap(barcodeString, Nindex);
	}
	return nullptr;
}

uint16 ProteinBarcodeProcessor::getProteinIndex(uint64 proteinBarcode){
	auto iter = proteinBarcodeMap->find(proteinBarcode);
    if (iter != proteinBarcodeMap->end()){
        return iter->second;
    }else{
        return 0xFFFF ;
    }
}

void ProteinBarcodeProcessor::misMaskGenerate()
{
	misMaskLen = possibleMis(barcodeLen, mismatch);
	misMaskLens = new int[mismatch];
	for (int i=0; i< mismatch; i++){
		misMaskLens[i] = possibleMis(barcodeLen, i+1);
	}
	
	misMask = (uint64*)malloc(misMaskLen * sizeof(uint64));
	set<uint64> misMaskSet;
	int index = 0;
	if (mismatch > 0) {
		for (int i = 0; i < barcodeLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt = j << i * 2;
				misMask[index] = misMaskInt;
				index++;
			}
		}
		if (mOptions->verbose) {
			string msg = "1 mismatch mask barcode number: " + to_string(index);
			loginfo(msg);
		}
	}
	if (mismatch == 2) {
		misMaskSet.clear();
		for (int i = 0; i < barcodeLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt1 = j << i * 2;
				for (int k = 0; k < barcodeLen; k++) {
					if (k == i) {
						continue;
					}
					for (uint64 j2 = 1; j2 < 4; j2++) {
						uint64 misMaskInt2 = j2 << k * 2;
						uint64 misMaskInt = misMaskInt1 | misMaskInt2;
						misMaskSet.insert(misMaskInt);
					}
				}
			}
		}
		for (auto iter = misMaskSet.begin(); iter != misMaskSet.end(); iter++) {
			misMask[index] = *iter;
			index++;
		}
		if (mOptions->verbose) {
			string msg = "2 mismatch mask barcode number: " + to_string(misMaskSet.size());
			loginfo(msg);
		}
	}
	if (mismatch == 3) {
		misMaskSet.clear();
		for (int i = 0; i < barcodeLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt1 = j << i * 2;
				for (int k = 0; k < barcodeLen; k++) {
					if (k == i) {
						continue;
					}
					for (uint64 j2 = 1; j2 < 4; j2++) {
						uint64 misMaskInt2 = j2 << k * 2;
						for (int h = 0; h < barcodeLen; h++) {
							if (h == k || h == i) {
								continue;
							}
							for (uint64 j3 = 1; j3 < 4; j3++) {
								uint64 misMaskInt3 = j3 << h * 2;
								uint64 misMaskInt = misMaskInt1 | misMaskInt2 | misMaskInt3;
								misMaskSet.insert(misMaskInt);
							}
						}
					}
				}
			}
		}
		for (auto iter = misMaskSet.begin(); iter != misMaskSet.end(); iter++) {
			misMask[index] = *iter;
			index++;
		}
		if (mOptions->verbose) {
			string msg = "3 mismatch mask barcode number: " + to_string(misMaskSet.size());
			loginfo(msg);
		}
		misMaskSet.clear();
	}
	if (mOptions->verbose) {
		string msg = "total mismatch mask length: " + to_string(misMaskLen);
		loginfo(msg);
	}
}

string ProteinBarcodeProcessor::positionToString(Position* position)
{
	stringstream positionString;
	positionString << position->x << "_" << position->y;
	return positionString.str();
}

string ProteinBarcodeProcessor::positionToString(Position1* position){
	stringstream positionString;
	positionString << position->x << "_" << position->y;
	return positionString.str();
}

unordered_map<uint64, Position1>::iterator ProteinBarcodeProcessor::getMisOverlap(uint64 barcodeInt)
{
	uint64 misBarcodeInt;
	int misCount = 0;
	int misMaskIndex = 0;
	unordered_map<uint64, Position1>::iterator iter;
	unordered_map<uint64, Position1>::iterator overlapIter;

	for (int mis = 0; mis < mismatch; mis++){
		misCount = 0;
		while (misMaskIndex < misMaskLens[mis]) {
			misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];
			misMaskIndex++;
			iter = bpmap->find(misBarcodeInt);
			if (iter != bpmap->end()) {
				overlapIter = iter;
				misCount++;
				if (misCount > 1) {
					return bpmap->end();
				}
			}
		}
		if (misCount == 1) {
			return overlapIter;
		}
	}
	return bpmap->end();
}

Position1* ProteinBarcodeProcessor::getNOverlap(string& barcodeString, uint8 Nindex)
{
	//N has the same encode (11) with G
	int misCount = 0;
	uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeString.length());
	unordered_map<uint64, Position1>::iterator iter;
	unordered_map<uint64, Position1>::iterator overlapIter;
	iter = bpmap->find(barcodeInt);
	if (iter!=bpmap->end()) {
		misCount++;
		overlapIter = iter;
	}
	for (uint64 j = 1; j < 4; j++) {
		uint64 misBarcodeInt = barcodeInt ^ (j << Nindex * 2);
		iter = bpmap->find(misBarcodeInt);
		if (iter != bpmap->end()) {
			misCount++;
			if (misCount > 1) {
				return nullptr;
			}
			overlapIter = iter;
		}
	}
	if (misCount == 1) {
		overlapReadsWithN++;
		return &overlapIter->second;
	}
	return nullptr;
}

int ProteinBarcodeProcessor::getNindex(string& barcodeString)
{
	int Nindex = barcodeString.find("N");
	if (Nindex == barcodeString.npos) {
		return -1;
	}
	else if (Nindex != barcodeString.rfind("N")) {
		return -2;
	}
	return Nindex;
}

bool ProteinBarcodeProcessor::barcodeStatAndFilter(pair<string, string>& barcode)
{
	for (int i = 0; i < barcodeLen; i++) {
		if (barcode.second[i] >= q30) {
			barcodeQ30++;
			barcodeQ20++;
			barcodeQ10++;
		}
		else if (barcode.second[i] >= q20) {
			barcodeQ20++;
			barcodeQ10++;
		}else if (barcode.second[i] >= q10) {
			barcodeQ10++;
		}
	}
	return true;
}

bool ProteinBarcodeProcessor::barcodeStatAndFilter(string& barcodeQ)
{
	for (int i = 0; i < barcodeLen; i++) {
		if (barcodeQ[i] >= q30) {
			barcodeQ30++;
			barcodeQ20++;
			barcodeQ10++;
		}
		else if (barcodeQ[i] >= q20) {
			barcodeQ20++;
			barcodeQ10++;
		}else if (barcodeQ[i] >= q10) {
			barcodeQ10++;
		}
	}
	return true;
}

bool ProteinBarcodeProcessor::umiStatAndFilter(pair<string, string>& umi)
{
	int q10BaseCount = 0;
	for (int i = 0; i < mOptions->transBarcodeToPos.umiLen; i++) {
		if (umi.second[i] >= q30) {
			umiQ30++;
			umiQ20++;
			umiQ10++;
		}
		else if (umi.second[i] >= q20) {
			umiQ20++;
			umiQ10++;
		}else if (umi.second[i] >= q10) {
			umiQ10++;
		}else{
			q10BaseCount++;
		}
	}
	if (umi.first.find("N") != string::npos){
		umiNFilterReads++;
		return false;
	}else if (seqEncode(umi.first.c_str(), 0, mOptions->transBarcodeToPos.umiLen) == 0){
		umiPloyAFilterReads++;
		return false;
	}else if (q10BaseCount>1){
		umiQ10FilterReads++;
		return false;
	}else{
		return true;
	}
}
