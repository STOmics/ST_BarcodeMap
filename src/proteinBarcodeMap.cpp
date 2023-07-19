#include "proteinBarcodeMap.h"

ProteinBarcodeMap::ProteinBarcodeMap(string& proteinBarcodeList, int mismatch, int start, int len){
    proteinBarcodeListFile = proteinBarcodeList;
    bStart = start;
    bLen = len;
    mMismatch = mismatch;
    loadProteinBarcodeMap();
}

ProteinBarcodeMap::~ProteinBarcodeMap(){

}

bool ProteinBarcodeMap::loadProteinBarcodeMap(){
    time_t start = time(NULL);
    if (! file_exists(proteinBarcodeListFile)){
        cerr << "protein barcode list file dose not exists: " << proteinBarcodeListFile << endl;
        exit(1);
    }

    cout << "###############load protein barcode list begin..." << endl;
    uint64 barcodeInt;
	uint16 proteinIndex;
    string proteinName;
	string line;
    ifstream listReader(proteinBarcodeListFile);
    while (std::getline(listReader, line)){
        if(line.empty()){
			cerr << "protein barcode list file read finished." << endl;
			break;
		}
        vector<string> splitLine;
        split(line, splitLine, "\t");
        if (splitLine.size() < 3){
			break;
		}
        proteinIndex = std::stoi(splitLine[0]);
        barcodeInt = seqEncode(splitLine[1].c_str(), 0, bLen);
        proteinName = splitLine[2];
        proteinNameMap[proteinIndex] = proteinName;
        proteinBarcodeMap[barcodeInt] = proteinIndex;
        if (mMismatch >= 1){
            generateMisProteinBarcodeMap(barcodeInt, proteinIndex);
        }
    }
    cout << "###############load protein barcode map finished, time used: " << time(NULL) - start << " seconds" << endl;
    cout << resetiosflags(ios::fixed) << setprecision(2);
	cout << "getProteinBarcodeMap_uniqBarcodeTypes: " << proteinBarcodeMap.size() << endl;
}

void ProteinBarcodeMap::generateMisProteinBarcodeMap(uint64 barcodeInt, uint16 proteinIndex){
   if (mMismatch >= 1){
       for (int i = 0; i < bLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt = j << i * 2;
                uint64 misBarcodeInt = barcodeInt^misMaskInt;
                if (proteinBarcodeMap.count(misBarcodeInt)>0){
                    string barcodestr = seqDecode(misBarcodeInt, bLen);
                    cerr << "origin protein index: " << proteinIndex << "\twith 1 mismatch barcode: " << barcodestr << "\tprotein Index: " << proteinBarcodeMap[misBarcodeInt] << endl;
                    cerr << "WARN: there are barcodes with hemming distance less 2" << endl;
                    exit(2);
                }else{
                    proteinBarcodeMap[misBarcodeInt] = proteinIndex;
                }
            }
        }
    }
    if(mMismatch >= 2){
        set<uint64> misBarcodeInts;
        for (int i = 0; i < bLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt1 = j << i * 2;
				for (int k = 0; k < bLen; k++) {
					if (k == i) {
						continue;
					}
					for (uint64 j2 = 1; j2 < 4; j2++) {
						uint64 misMaskInt2 = j2 << k * 2;
						uint64 misMaskInt = misMaskInt1 | misMaskInt2;
						uint64 misBarcodeInt = barcodeInt^misMaskInt;
                        if (misBarcodeInts.count(misBarcodeInt) > 0){
                            continue;
                        }
                        misBarcodeInts.insert(misBarcodeInt);
                        if (proteinBarcodeMap.count(misBarcodeInt)>0){
                            string originBarcodestr = seqDecode(barcodeInt, bLen);
                            string barcodestr = seqDecode(misBarcodeInt, bLen);
                            cerr << "map size: " << proteinBarcodeMap.size() << endl;
                            cerr << "origin protein sequence: " << originBarcodestr << "\torigin protein index: " << proteinIndex << "\twith 2 mismatch barcode: " << barcodestr << "\tprotein Index: " << proteinBarcodeMap[misBarcodeInt] << endl;
                            cerr << "WARN: there are barcodes with hemming distance less 2" << endl;
                            exit(2);
                        }else{
                            proteinBarcodeMap[misBarcodeInt] = proteinIndex;
                        }
					}
				}
			}
		}
    }
}

uint16 ProteinBarcodeMap::getProteinIndex(uint64 barcodeInt){
    auto iter = proteinBarcodeMap.find(barcodeInt);
    if (iter != proteinBarcodeMap.end()){
        return iter->second;
    }else{
        return -1;
    }
}

string ProteinBarcodeMap::getProteinName(uint16 proteinIndex){
    return proteinNameMap[proteinIndex];
}

unordered_map<uint64, uint16>* ProteinBarcodeMap::getProteinBarcodeMap(){
    return &proteinBarcodeMap;
}

unordered_map<uint16, string>* ProteinBarcodeMap::getProteinNameMap(){
    return &proteinNameMap;
}