#ifndef CHIPMASKMERGE_H
#define CHIPMASKMERGE_H

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <hdf5.h>
#include <unordered_map>
#include <iomanip>
#include <set>
#include "common.h"
#include "util.h"
#include "chipMaskHDF5.h"
#include "options.h"
#include "htmlreporter.h"

using namespace std;

class ChipMaskMerge{
public:
    ChipMaskMerge(Options* opt);
    ~ChipMaskMerge();
    void maskMerge();
private:
    void dumpBpmap();
    bool add(uint64 barcodeInt, Position1& position);
    void rangeRefresh(Position1& position);
public:
    Options* mOptions;
    vector<std::string> inMasks;
    std::string outMask;
    unordered_map<uint64, Position1> bpmap;
    set<uint64> dupBarcode;
    long dupBarcodes = 0;
    long overlapBarcodes = 0;
    long totalBarcodes = 0;
    uint32 barcodeLen;
    uint32 minX = OUTSIDE_DNB_POS_COL;
    uint32 minY = OUTSIDE_DNB_POS_ROW;
    uint32 maxX = 0;
    uint32 maxY = 0;
    int slidePitch = 500;
    int segment = 1;
};

#endif //! CHIPMASKMERGE_H