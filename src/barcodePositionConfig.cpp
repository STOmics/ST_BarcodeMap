#include "barcodePositionConfig.h"

BarcodePositionConfig::BarcodePositionConfig(Options* opt, int threadId)
{
	mOptions = opt;
	totalReads = 0;
	lowQReads = 0;
	dupReads = 0;
	withoutPositionRead = 0;
	mThreadId = threadId;
}

BarcodePositionConfig::~BarcodePositionConfig()
{
	if (!barcodeSet.empty()) {
		barcodeSet.clear();
		unordered_set<uint64>().swap(barcodeSet);
	}
	if (!bpmap.empty()) {
		bpmap.clear();
		unordered_map<uint64, Position1>().swap(bpmap);
	}
}

BarcodePositionConfig* BarcodePositionConfig::merge(BarcodePositionConfig** configs)
{
	if (sizeof(configs) == 0) {
		return nullptr;
	}
	BarcodePositionConfig* config = new BarcodePositionConfig(configs[0]->mOptions, 0);
	for (int i = 0; i < sizeof(configs) / sizeof(BarcodePositionConfig*); i++) {
		config->totalReads += configs[i]->totalReads;
		config->lowQReads += configs[i]->lowQReads;
		config->dupReads += configs[i]->dupReads;
		config->withoutPositionRead += configs[i]->withoutPositionRead;
		for (auto iter = configs[i]->bpmap.begin(); iter != configs[i]->bpmap.end(); iter++) {
			if (config->barcodeSet.count(iter->first) == 0) {
				config->barcodeSet.insert(iter->first);
				config->bpmap[iter->first] = iter->second;
			}
			else if(config->bpmap.count(iter->first)>0){
				config->bpmap.erase(iter->first);
			}
		}
		configs[i]->barcodeSet.clear();
		unordered_set<uint64>().swap(configs[i]->barcodeSet);
		configs[i]->bpmap.clear();
		unordered_map<uint64, Position1>().swap(configs[i]->bpmap);
	}
	return config;
}

void BarcodePositionConfig::addBarcode(uint64 barcodeInt, Position1& pos)
{
	if (barcodeSet.count(barcodeInt) == 0) {
		barcodeSet.insert(barcodeInt);
		bpmap[barcodeInt] = pos;
	}
	else if (bpmap.count(barcodeInt) > 0) {
		dupReads++;
		bpmap.erase(barcodeInt);
	}
}

void BarcodePositionConfig::print()
{
	cout << "Total Reads:\t" << totalReads << endl;
	cout << "LowQ reads:\t" << lowQReads << endl;
	cout << "Duplicated reads:\t" << dupReads << endl;
	cout << "Barcode types:\t" << barcodeSet.size() << endl;
	cout << "Unique barcode types:\t" << bpmap.size() << endl;
}
