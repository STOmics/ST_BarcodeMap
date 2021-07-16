#include "chipMaskFormatChange.h"

ChipMaskFormatChange::ChipMaskFormatChange(Options* opt){
    mOptions = opt;
    inMask = mOptions->in;
    outMask = mOptions->out;
    barcodeStart = mOptions->barcodeStart;
    barcodeLen = mOptions->barcodeLen;
}

ChipMaskFormatChange::~ChipMaskFormatChange(){
	bpmap.clear();
	unordered_map<uint64, Position1>().swap(bpmap);
}

void ChipMaskFormatChange::rangeRefresh(Position1& position){
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

void ChipMaskFormatChange::change(){
	loadbpmap();
	dumpbpmap();
}

void ChipMaskFormatChange::loadbpmap()
{
	time_t start = time(NULL);
	if (! file_exists(inMask)){
		cerr << "barcodePositionMapFile does not exists: " << inMask <<endl;
		exit(1);
	}
	
	cout << "###############load barcodeToPosition map begin..." << endl;
	//cout << "###############barcode map file: " << barcodePositionMapFile << endl;
	if (ends_with(inMask, ".bin")) {		
		ifstream mapReader(inMask, ios::in | ios::binary);
		if (! mapReader.is_open()){
			throw invalid_argument("Could not open the file: " + inMask);
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
	else if (ends_with(inMask, "h5") || ends_with(inMask, "hdf5")){
		ChipMaskHDF5 chipMaskH5(inMask);
		chipMaskH5.openFile();
		chipMaskH5.readDataSet(bpmap);
	}
	else {
		uint64 barcodeInt;
		Position1 position;
		string line;
		ifstream mapReader(inMask);
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

void ChipMaskFormatChange::dumpbpmap() {
	time_t start = time(NULL);
	cout << "##########dump barcodeToPosition map begin..." << endl;
	if (ends_with(outMask, ".bin")) {
		unordered_map<uint64, Position1>::iterator mapIter = bpmap.begin();
		bpmap.reserve(bpmap.size());
		ofstream writer(outMask, ios::out | ios::binary);
		//boost::archive::binary_oarchive oa(writer);
		//oa << bpmap;
		while (mapIter != bpmap.end()) {
			writer.write((char*)&mapIter->first, sizeof(uint64));
			writer.write((char*)&mapIter->second, sizeof(uint32));
			writer.write((char*)&mapIter->second, sizeof(uint32));
			mapIter++;
		}
		writer.close();
	}else if (ends_with(outMask, "h5") || ends_with(outMask, "hdf5")){
		ChipMaskHDF5 chipMaskH5(outMask);
		chipMaskH5.creatFile();
		uint8_t segment = mOptions->barcodeSegment;
		if (mOptions->rc == 2){
			segment *= 2;
		}
		slideRange sliderange = {minX, maxX, minY, maxY};
		chipMaskH5.writeDataSet(mOptions->chipID, sliderange, bpmap, barcodeLen, segment, slidePitch, mOptions->compression);
	}
	else {
		ofstream writer(outMask);
		unordered_map<uint64, Position1>::iterator mapIter = bpmap.begin();
		while (mapIter != bpmap.end()) {
			writer << seqDecode(mapIter->first, barcodeLen) << "\t" << mapIter->second.x << "\t" << mapIter->second.y << endl;
			mapIter++;
		}
		writer.close();
	}	
	cout << "##########dump barcodeToPosition map finished, time used: " << time(NULL) - start << " seconds" << endl;
}