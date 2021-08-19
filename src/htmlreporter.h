#ifndef HTML_REPORTER_H
#define HTML_REPORTER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "options.h"
#include <fstream>
#include <sstream>
#include "barcodePositionMap.h"

using namespace std;

class HtmlReporter{
public:
    HtmlReporter(Options* opt);
    ~HtmlReporter();
    
    static void outputRow(ofstream& ofs, string key, long value);
    static void outputRow(ofstream& ofs, string key, string value);
    static string formatNumber(long number);
    static string toThousands(long number);
    static string getPercents(long numerator, long denominator);
    void printReport(string htmlFile, long totalBarcodes, long overlapBarcodes, long dupBarcodes, long uniqueBarcodes, vector<string>& maskFileList);
private:
    const string getCurrentSystemTime();
    void printHeader(ofstream& ofs);
    void printCSS(ofstream& ofs);
    void printJS(ofstream& ofs);
    void printFooter(ofstream& ofs);
	void reportBarcode(ofstream& ofs, long* barcodeStat, int maxReadsNumber);
    void printSummary(ofstream& ofs, long totalBarcodes, long overlapBarcodes, long dupBarcodes, long uniqueBarcodes, vector<string>& maskFileList);
    
    string list2string(long* list, int size);
    
private:
    Options* mOptions;
};


#endif