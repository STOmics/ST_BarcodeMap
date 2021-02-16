#include "barcodePositionMap.h"

BarcodePositionMap::BarcodePositionMap(Options* opt)
{
	mOptions = opt;
	split(opt->in, inFile, ",");
	maskFile = opt->maskFile;
	barcodeStart = opt->barcodeStart;
	barcodeLen = opt->barcodeLen;
	segment = opt->barcodeSegment;
	turnFovDegree = opt->turnFovDegree;
	isSeq500 = opt->isSeq500;
	rc = opt->rc;
	//polyTint = getPolyTint(barcodeLen);
	readidSep = opt->barcodeStat.readidSep;
	inFastqNumber = inFile.size();
	firstFastq = *inFile.begin();
	initiate();
	loadbpmap();
}

BarcodePositionMap::~BarcodePositionMap()
{
	bpmap.clear();
	unordered_map<uint64, Position1>().swap(bpmap);
	dupBarcode.clear();
	set<uint64>().swap(dupBarcode);
	inFile.clear();
	vector<string>().swap(inFile);
	delete[] totalReads;
	delete[] readsWithN;
	delete[] dupReads;
	delete[] ESTdupReads;
	for (int i=0; i<inFastqNumber; i++){
		delete[] polyReads[i];
	}
	delete[] polyReads;
	delete[] readsQ10;
	delete[] readsQ20;
	delete[] readsQ30;
	delete[] totalBase;
	delete[] totalBarcodes;
}

void BarcodePositionMap::initiate(){
	totalReads = new long[inFastqNumber]();
	readsWithN = new long[inFastqNumber]();
	readsWithoutPos = new long[inFastqNumber]();
	polyReads = new long*[inFastqNumber]();
	for (int i=0; i<inFastqNumber; i++){
		polyReads[i] = new long[4]();
	}
	dupReads = new long[inFastqNumber]();
	ESTdupReads = new long[inFastqNumber]();
	readsQ10 = new long[inFastqNumber]();
	readsQ20 = new long[inFastqNumber]();
	readsQ30 = new long[inFastqNumber]();
	totalBase = new long[inFastqNumber]();
	totalBarcodes = new long[inFastqNumber]();
}

long BarcodePositionMap::getBarcodeTypes()
{
	return bpmap.size();
}

void BarcodePositionMap::dumpbpmap(string& mapOutFile) {
	time_t start = time(NULL);
	cout << "##########dump barcodeToPosition map begin..." << endl;
	if (ends_with(mapOutFile, ".bin")) {
		bpmap.reserve(bpmap.size());
		ofstream writer(mapOutFile, ios::out | ios::binary);
		boost::archive::binary_oarchive oa(writer);
		oa << bpmap;
		//while (mapIter != bpmap.end()) {
		//	writer.write((char*)&mapIter->first, sizeof(uint64));
		//	writer.write((char*)&mapIter->second, sizeof(Position));
		//	mapIter++;
		//}
		writer.close();
	}else if (ends_with(mapOutFile, "h5") || ends_with(mapOutFile, "hdf5")){
		ChipMaskHDF5 chipMaskH5(mapOutFile);
		chipMaskH5.creatFile();
		int segment = mOptions->barcodeSegment;
		if (mOptions->rc == 2){
			segment *= 2;
		}
		chipMaskH5.writeDataSet(mOptions->chipID, sliderange, bpmap, barcodeLen, segment, slidePitch, mOptions->compression);
	}
	else {
		ofstream writer(mapOutFile);
		unordered_map<uint64, Position1>::iterator mapIter = bpmap.begin();
		while (mapIter != bpmap.end()) {
			writer << seqDecode(mapIter->first, barcodeLen) << "\t" << mapIter->second.x << "\t" << mapIter->second.y << endl;
			mapIter++;
		}
		writer.close();
	}	
	cout << "##########dump barcodeToPosition map finished, time used: " << time(NULL) - start << " seconds" << endl;
}

void BarcodePositionMap::loadbpmap()
{
	time_t start = time(NULL);
	string barcodePositionMapFile = inFile.at(0);
	if (! file_exists(barcodePositionMapFile)){
		cerr << "barcodePositionMapFile does not exists: " << barcodePositionMapFile <<endl;
		exit(1);
	}
	
	cout << "###############load barcodeToPosition map begin..." << endl;
	//cout << "###############barcode map file: " << barcodePositionMapFile << endl;
	if (ends_with(barcodePositionMapFile, ".bin")) {
		ifstream mapReader(barcodePositionMapFile, ios::in | ios::binary);
		if (! mapReader.is_open()){
			throw invalid_argument("Could not open the file: " + barcodePositionMapFile);
		}
		boost::archive::binary_iarchive ia(mapReader);
		ia >> bpmap;
		//while (!mapReader.eof()) {
		//	mapReader.read((char*)&barcodeInt, sizeof(barcodeInt));
		//	mapReader.read((char*)&position, sizeof(position));
		//	bpmap[barcodeInt] = position;
		//}
		mapReader.close();
	}
	else if (ends_with(barcodePositionMapFile, "h5") || ends_with(barcodePositionMapFile, "hdf5")){
		ChipMaskHDF5 chipMaskH5(barcodePositionMapFile);
		chipMaskH5.openFile();
		chipMaskH5.readDataSet(bpmap);
	}
	else {
		uint64 barcodeInt;
		Position1 position;
		string line;
		ifstream mapReader(barcodePositionMapFile);
		while (std::getline(mapReader, line)) {
			if(line.empty()){
				cerr << "barcodePositionMap file read finished." << endl;
				break;
			}
			vector<string> splitLine;
			//std::getline(mapReader, line);
			split(line, splitLine, "\t");
			//position.fov_c = fovCol;
			//position.fov_r = fovRow;
			if (splitLine.size() < 3){
				break;
			}
			else if (splitLine.size() == 3){
				position.x = std::stoi(splitLine[1]);
				position.y = std::stoi(splitLine[2]);
			}else {
				position.x = std::stoi(splitLine[3]);
				position.y = std::stoi(splitLine[4]);
			}
			barcodeInt = seqEncode(splitLine[0].c_str(), barcodeStart, barcodeLen);
			bpmap[barcodeInt] = position;
			//cout << "barcode: " << barcodeInt << " position: " << position.x << " " << position.y <<endl;
		}
		//cout << "bpmap load suceessfully." << endl;
		mapReader.close();
	}
	cout << "###############load barcodeToPosition map finished, time used: " << time(NULL) - start << " seconds" << endl;
	cout << resetiosflags(ios::fixed) << setprecision(2);
	cout << "getBarcodePositionMap_uniqBarcodeTypes: " << bpmap.size() << endl;
}

void BarcodePositionMap::getSuffixLen(){
	FastqReader reader(firstFastq);
	Read* read = reader.read();
	string readName = read->mName;
	int sepPos = readName.find(readidSep);
	if (sepPos!=readName.npos){
		suffixLen = readName.size() - sepPos;
	}else{
		suffixLen = 0;
	}
	
}

int BarcodePositionMap::readQualityStat(string& readQ, int index){
	int lowQ10 = 0;
	for (int i = 0; i < readQ.size(); i++){
		totalBase[index]++;
		if (readQ[i] >= 30+33){
			readsQ30[index]++;
			readsQ20[index]++;
			readsQ10[index]++;
		}else if (readQ[i] >= 20+33){
			readsQ20[index]++;
			readsQ10[index]++;
		}else if (readQ[i] >= 10+33){
			readsQ10[index]++;
		}else{
			lowQ10++;
		}
	}
	return lowQ10;
}

/**
 * baseCount: ['A', 'C', 'T', 'G']
*/
bool BarcodePositionMap::barcodeFilter(string& readSeq, int index){
	int baseCount[5] = {0, 0, 0, 0, 0};
	int readLen = readSeq.size();
	for (int i = 0; i < readLen; i++){
		switch (readSeq[i]){
			case 'A':
				baseCount[0]++;
				break;
			case 'C':
				baseCount[1]++;
				break;
			case 'T':
				baseCount[2]++;
				break;
			case 'G':
				baseCount[3]++;
				break;
			default:
				baseCount[4]++;
		}
	}
	if (baseCount[4]>0){
		readsWithN[index]++;
		return true;
	}
	for (int i=0; i<4; i++){
		float polyRate = baseCount[i]/readLen;
		if (polyRate>=0.8){
			polyReads[index][i]++;
			return true;
		}
	}
	return false;
}

