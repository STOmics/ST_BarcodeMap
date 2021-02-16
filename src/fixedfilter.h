#ifndef FIXED_FILTER_H
#define FIXED_FILTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <fstream>
#include "read.h"
#include "result.h"
#include "options.h"
#include "util.h"

using namespace std;

class FixedFilter {
public:
	FixedFilter(Options* opt);
	~FixedFilter();
	bool filter(Read* read1, Read* read2, Result* result);
	bool filterBySequence(Read* read1, Read* read2, Result* result);
	bool filterByPosSpecifySequence(Read* read1, Read* read2, Result* result, int posStart);
	bool filterBySequences(Read* read1, Read* read2, Result* result, string& fixedSequence);
	bool filterByPosSpecifySequences(Read* read1, Read* read2, Result* result, string& fixedSequence, int posStart);
	bool filterByMultipleSequences(Read* read1, Read* read2, Result* result);
	void getFxiedSequencesFromFile(string fixedSequenceFile);
public:
	Options* mOptions;
	string fixedSequence;
	string fixedSequenceRC;
	map<string, int> fixedSequences;
	int read1Length = 50;
};

#endif