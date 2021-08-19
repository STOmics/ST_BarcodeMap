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
#include "barcodePositionMap.h"

using namespace std;

class ChipMaskFormatChange{

public:
    ChipMaskFormatChange(Options* opt);
    ~ChipMaskFormatChange();

    void change();
    void H5ToBin();
public:
    Options* mOptions;
    BarcodePositionMap* bpmap;
};

#endif // ! CHIPMASKFORMATCHANGE_H