#ifndef GENERATE_GEM_H
#define GENERATE_GEM_H

#include <iostream>
#include <unordered_map>
#include <istream>
#include <fstream>
#include <iomanip>
#include "common.h"
#include "util.h"

using namespace std;

class GenerateGem{
public:
    GenerateGem(unordered_map<uint64, unordered_map<uint64, uint32>>* GemMap, int umiLength);
    ~GenerateGem();

    void umiCorrection();
    int umiDistance(uint64 umi1, uint64 umi2);
    void dumpGem(string rawGemFile, string genFile, unordered_map<uint16, string>& proteinNameMap);
    //bool compareBySecond(const pair< uint64, uint32 >& p1, const pair< uint64, uint32 >& p2);

public:
    unordered_map<uint64, unordered_map<uint64, uint32>>* gemMap;
    int umiLen;
private:
    int numThre = 5;
    int mismatch = 1;
};


#endif // ! GENERATE_GEM_H