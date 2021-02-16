#ifndef BARCODELISTMERGE_H
#define BARCODELISTMERGE_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "options.h"

using namespace std;

class BarcodeListMerge{
public:
    BarcodeListMerge(Options* opt);
    ~BarcodeListMerge();
    void mergeBarcodeLists();
private:
    void addBarcodeList(unordered_map<uint64, int>& dnbMap);
    void dumpMergedBarcodeList(string& outfile);
private:
    Options* mOptions;
    vector<string> dnbMapFiles;
    string mergedDnbMapFile;
    unordered_map<uint64, int> mergedDnbMap;
};

#endif 