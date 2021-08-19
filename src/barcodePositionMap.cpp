#include "barcodePositionMap.h"

BarcodePositionMap::BarcodePositionMap(Options* opt)
{
	mOptions = opt;
	maskFile = opt->maskFile;
	barcodeStart = opt->barcodeStart;
	barcodeLen = opt->barcodeLen;
	segment = opt->barcodeSegment;
	split(opt->in, inMasks, ",");
	loadbpmap();
}

BarcodePositionMap::~BarcodePositionMap()
{
	bpmap.clear();
	unordered_map<uint64, Position1>().swap(bpmap);
	dupBarcode.clear();
	set<uint64>().swap(dupBarcode);
}

void BarcodePositionMap::rangeRefresh(Position1& position){
	if (position.x < minX){
		minX  = position.x;
	}else if (position.x > maxX){
		maxX = position.x;
	}
	if (position.y < minY){
		minY = position.y;
	}else if (position.y > maxY){
		maxY = position.y;
	}
}

long BarcodePositionMap::getBarcodeTypes()
{
	return bpmap.size();
}

void BarcodePositionMap::dumpbpmap(string& mapOutFile) {
	time_t start = time(NULL);
	cout << "##########dump barcodeToPosition map begin..." << endl;
	if (ends_with(mapOutFile, ".bin")) {
		unordered_map<uint64, Position1>::iterator mapIter = bpmap.begin();
		bpmap.reserve(bpmap.size());
		ofstream writer(mapOutFile, ios::out | ios::binary);
		//boost::archive::binary_oarchive oa(writer);
		//oa << bpmap;
		while (mapIter != bpmap.end()) {
			writer.write((char*)&mapIter->first, sizeof(uint64));
			writer.write((char*)&mapIter->second, sizeof(uint32));
			writer.write((char*)&mapIter->second, sizeof(uint32));
			mapIter++;
		}
		writer.close();
	}else if (ends_with(mapOutFile, "h5") || ends_with(mapOutFile, "hdf5")){
		ChipMaskHDF5 chipMaskH5(mapOutFile);
		chipMaskH5.creatFile();
		uint8_t segment = mOptions->barcodeSegment;
		if (mOptions->rc == 2){
			segment *= 2;
		}
		slideRange sliderange{minX, maxX, minY, maxY};
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
	string barcodePositionMapFile = inMasks.at(0);
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
		//boost::archive::binary_iarchive ia(mapReader);
		//ia >> bpmap;
		uint64 barcodeInt;
		Position1 position;
		while (!mapReader.eof()) {
			mapReader.read((char*)&barcodeInt, sizeof(barcodeInt));
			mapReader.read((char*)&position.x, sizeof(position.x));
			mapReader.read((char*)&position.y, sizeof(position.y));
			bpmap[barcodeInt] = position;
			rangeRefresh(position);
		}
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
			rangeRefresh(position);
			//cout << "barcode: " << barcodeInt << " position: " << position.x << " " << position.y <<endl;
		}
		//cout << "bpmap load suceessfully." << endl;
		mapReader.close();
	}
	cout << "###############load barcodeToPosition map finished, time used: " << time(NULL) - start << " seconds" << endl;
	cout << resetiosflags(ios::fixed) << setprecision(2);
	cout << "getBarcodePositionMap_uniqBarcodeTypes: " << bpmap.size() << endl;
}

