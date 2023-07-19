#ifndef PROTEINBARCODEMAP_H
#define PROTEINBARCODEMAP_H

#include <string>
#include <unordered_map>
#include <set>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "util.h"

using namespace std;

class ProteinBarcodeMap {
public:
    ProteinBarcodeMap(string& proteinBarcodeList, int mismatch = 1, int start=0, int len=15);
    ~ProteinBarcodeMap();

public:
    bool loadProteinBarcodeMap();
    uint16 getProteinIndex(uint64 proteinBarcodeInt);
    string getProteinName(uint16 proteinIndex);
    void generateMisProteinBarcodeMap(uint64 barcodeInt, uint16 proteinIndex);
    unordered_map<uint64, uint16>* getProteinBarcodeMap();
    unordered_map<uint16, string>* getProteinNameMap();

public:
    string proteinBarcodeListFile;
    unordered_map<uint64, uint16> proteinBarcodeMap;
    unordered_map<uint16, string> proteinNameMap;
    int bStart;
    int bLen;
    int mMismatch;
};

#endif // ! PROTEINBARCODEMAP_H