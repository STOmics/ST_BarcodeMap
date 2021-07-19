#ifndef CHIPMASKFORMATCHANGE_H
#define CHIPMASKFORMATCHANGE_H

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <hdf5.h>
#include <unordered_map>
#include <iomanip>
#include "common.h"
#include "util.h"
#include "chipMaskHDF5.h"
#include "options.h"

using namespace std;

class ChipMaskFormatChange{

public:
    ChipMaskFormatChange(Options* opt);
    ~ChipMaskFormatChange();

    void change();
    void loadbpmap();
    void dumpbpmap();
private:
    void rangeRefresh(Position1& position);
public:
    Options* mOptions;
    std::string inMask;
    std::string outMask;
    unordered_map<uint64, Position1> bpmap;
    int barcodeStart;
    int barcodeLen;
    uint32 minX = OUTSIDE_DNB_POS_COL;
    uint32 minY = OUTSIDE_DNB_POS_ROW;
    uint32 maxX = 0;
    uint32 maxY = 0;
    uint32_t slidePitch=500;
};

#endif // ! CHIPMASKFORMATCHANGE_H