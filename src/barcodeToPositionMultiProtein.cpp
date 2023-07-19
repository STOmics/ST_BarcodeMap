#include "barcodeToPositionMultiProtein.h"

BarcodeToPositionMultiProtein::BarcodeToPositionMultiProtein(Options* opt)
{
	mOptions = opt;
	mProduceFinished = false;
	mFinishedThreads = 0;
	mOutStream = NULL;
	mZipFile = NULL;
	mapInserter = NULL;
	mUnmappedWriter1 = NULL;
	mUnmappedWriter2 = NULL;
	bool isSeq500 = opt->isSeq500;
	split(mOptions->transBarcodeToPos.in1, in1);
	split(mOptions->transBarcodeToPos.in2, in2);
	proteinBarcodeMap = new ProteinBarcodeMap(opt->transBarcodeToPos.proteinBarcodeList, opt->transBarcodeToPos.proteinBarcodeMismatch, opt->transBarcodeToPos.proteinBarcodeStart, opt->transBarcodeToPos.proteinBarcodeLen);
	mbpmap = new BarcodePositionMap(opt);
	//barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
	if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
		fixedFilter = new FixedFilter(opt);
		filterFixedSequence = true;
	}
}

BarcodeToPositionMultiProtein::~BarcodeToPositionMultiProtein()
{
	//if (fixedFilter) {
	//	delete fixedFilter;
	//}
	//unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMultiProtein::process()
{
	initOutput();
	initPackRepositoey();
	std::thread producer(std::bind(&BarcodeToPositionMultiProtein::producerTask, this));

	ProteinResult** results = new ProteinResult*[mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		results[t] = new ProteinResult(mOptions, true);
		results[t]->setBarcodeProcessor(mbpmap->getBpmap(), proteinBarcodeMap->getProteinBarcodeMap());
	}

	std::thread** threads = new thread * [mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t] = new std::thread(std::bind(&BarcodeToPositionMultiProtein::consumerTask, this, results[t]));
	}

	std::thread* mapInsertThread = NULL;
	std::thread* unMappedWriterThread1 = NULL;
	std::thread* unMappedWriterThread2 = NULL;
	if(mapInserter){
		mapInsertThread = new std::thread(std::bind(&BarcodeToPositionMultiProtein::mapInsertTask, this, mapInserter));
	}
	if (mUnmappedWriter1 && mUnmappedWriter2) {
		unMappedWriterThread1 = new std::thread(std::bind(&BarcodeToPositionMultiProtein::writeTask, this, mUnmappedWriter1));
		unMappedWriterThread2 = new std::thread(std::bind(&BarcodeToPositionMultiProtein::writeTask, this, mUnmappedWriter2));
		
	}

	producer.join();
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t]->join();
	}

	if (mapInsertThread)
		mapInsertThread->join();
	if (unMappedWriterThread1)
		unMappedWriterThread1->join();
	if (unMappedWriterThread2)
		unMappedWriterThread2->join();

	if (mOptions->verbose)
		loginfo("start to generate reports\n");

	//merge result
	vector<ProteinResult*> resultList;
	for (int t = 0; t < mOptions->thread; t++){
		resultList.push_back(results[t]);
	}
	ProteinResult* finalResult = ProteinResult::merge(resultList);
	finalResult->print();

	//dump gene expression matrix
	GenerateGem* generateGem = new GenerateGem(&mapInserter->gemMap, mOptions->transBarcodeToPos.umiLen);
	generateGem->umiCorrection();
	string gemFile = mOptions->transBarcodeToPos.out1;
	string rawGemFile = mOptions->transBarcodeToPos.out2;
	generateGem->dumpGem(rawGemFile, gemFile, proteinBarcodeMap->proteinNameMap);

	cout << resetiosflags(ios::fixed) << setprecision(2);
	
	//clean up
	for (int t = 0; t < mOptions->thread; t++) {
		delete threads[t];
		threads[t] = NULL;
		delete results[t];
		results[t] = NULL;
	}

	delete[] threads;
	delete[] results;

	if (mapInsertThread)
		delete mapInsertThread;
	if (unMappedWriterThread1)
		delete unMappedWriterThread1;
	if (unMappedWriterThread2)
		delete unMappedWriterThread2;

	closeOutput();

	return true;
}

void BarcodeToPositionMultiProtein::initOutput() {
	mapInserter = new MapInsertThread();
	if (!mOptions->transBarcodeToPos.unmappedOutFile.empty() && !mOptions->transBarcodeToPos.unmappedOutFile2.empty()) {
		mUnmappedWriter1 = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile, mOptions->compression);
		mUnmappedWriter2 = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile2, mOptions->compression);
	}
}

void BarcodeToPositionMultiProtein::closeOutput()
{
	if (mapInserter) {
		delete mapInserter;
		mapInserter = NULL;
	}
	if (mUnmappedWriter1) {
		delete mUnmappedWriter1;
		mUnmappedWriter1 = NULL;
	}
	if (mUnmappedWriter2) {
		delete mUnmappedWriter2;
		mUnmappedWriter2 = NULL;
	}
}

bool BarcodeToPositionMultiProtein::processPairEnd(ReadPairPack2* pack, ProteinResult* result)
{
	vector<uint64> outRecords;
	string unmappedOut1;
	string unmappedOut2;
	bool hasPosition;
	bool fixedFiltered;
	for (int p = 0; p < pack->count; p++) {
		result->mTotalRead++;
		ReadPair* pair = pack->data[p];
		Read* or1 = pair->mLeft;
		Read* or2 = pair->mRight;
		
		hasPosition = result->mBarcodeProcessor->process(or1, or2);
		if (hasPosition) {
			uint64 key = encodeCIDGene(or1->x, or1->y, or2->proteinIndex);
			uint64 value = or1->umiInt;
			outRecords.push_back(key);
			outRecords.push_back(value);
		}
		else if (mUnmappedWriter1 && mUnmappedWriter2) {
			unmappedOut1 += or1->toString();
			unmappedOut2 += or2->toString();
		}
		delete pair;
	}
	mOutputMtx.lock();
	if (mUnmappedWriter1 && mUnmappedWriter2 && (!unmappedOut1.empty() || !unmappedOut2.empty())) {
		//write reads that can't be mapped to the slide
		char* udata1 = new char[unmappedOut1.size()];
		memcpy(udata1, unmappedOut1.c_str(), unmappedOut1.size());
		mUnmappedWriter1->input(udata1, unmappedOut1.size());

		char* udata2 = new char[unmappedOut2.size()];
		memcpy(udata2, unmappedOut2.c_str(), unmappedOut2.size());
		mUnmappedWriter2->input(udata2, unmappedOut2.size());
	}
	
	if (mapInserter && !outRecords.empty()) {
		uint64* data = new uint64[outRecords.size()];
		std::copy(outRecords.begin(), outRecords.end(), data);
		mapInserter->input(data, outRecords.size());
	}
	mOutputMtx.unlock();
	delete pack->data;
	delete pack;
	return true;
}

void BarcodeToPositionMultiProtein::initPackRepositoey()
{
	mRepo.packBuffer = new ReadPairPack2 * [PACK_NUM_LIMIT];
	memset(mRepo.packBuffer, 0, sizeof(ReadPairPack2*) * PACK_NUM_LIMIT);
	mRepo.writePos = 0;
	mRepo.readPos = 0;
}

void BarcodeToPositionMultiProtein::destroyPackRepository() {
	delete mRepo.packBuffer;
	mRepo.packBuffer = NULL;
}

void BarcodeToPositionMultiProtein::producePack(ReadPairPack2* pack) {
	mRepo.packBuffer[mRepo.writePos] = pack;
	mRepo.writePos++;
}

void BarcodeToPositionMultiProtein::consumePack(ProteinResult* result) {
	ReadPairPack2* data;
	mInputMutx.lock();
	while (mRepo.writePos <= mRepo.readPos) {
		usleep(1000);
		if (mProduceFinished) {
			mInputMutx.unlock();
			return;
		}
	}
	data = mRepo.packBuffer[mRepo.readPos];
	mRepo.readPos++;
	//if (mOptions->verbose)
	//	loginfo("readPos: " + to_string(mRepo.readPos) + "\n");
	mInputMutx.unlock();
	processPairEnd(data, result);
}

void BarcodeToPositionMultiProtein::producerTask() {
	if (mOptions->verbose)
		loginfo("start to load data");
	long lastReported = 0;
	int slept = 0;
	long readNum = 0;
	ReadPair** data = new ReadPair * [PACK_SIZE];
	memset(data, 0, sizeof(ReadPair*) * PACK_SIZE);
	int count = 0;
	bool needToBreak = false;
	FastqReaderPair* readers[in1.size()];
	for (int i = 0; i<in1.size(); i++){
		string fq1 = in1[i];
		string fq2 = in2[i];
		readers[i] = new FastqReaderPair(fq1, fq2, true);
		//cout << "########begin to read fastq file: " << endl;
		while (true) {
			ReadPair* read = readers[i]->read();
			if (!read && i < in1.size()-1) {
				break;
			}
			if  (!read || needToBreak) {
				ReadPairPack2* pack = new ReadPairPack2;
				pack->data = data;
				pack->count = count;
				producePack(pack);
				data = NULL;
				if (read) {
					delete read;
					read = NULL;
				}
				break;
			}
			data[count] = read;
			count++;
			if (mOptions->verbose && count + readNum >= lastReported + 1000000) {
				lastReported = count + readNum;
				string msg = "loaded " + to_string((lastReported / 1000000)) + "M read pairs";
				loginfo(msg);
			}
			if (count == PACK_SIZE || needToBreak) {
				ReadPairPack2* pack = new ReadPairPack2;
				pack->data = data;
				pack->count = count;
				producePack(pack);
				//re-initialize data for next pack
				data = new ReadPair * [PACK_SIZE];
				memset(data, 0, sizeof(ReadPair*) * PACK_SIZE);
				// if the consumer is far behind this producer, sleep and wait to limit memory usage
				while (mRepo.writePos - mRepo.readPos > PACK_IN_MEM_LIMIT) {
					slept++;
					usleep(100);
				}
				readNum += count;
				// if the writer threads are far behind this producer, sleep and wait
				// check this only when necessary
				if (readNum % (PACK_SIZE * PACK_IN_MEM_LIMIT) == 0 && mapInserter) {
					while (mapInserter && mapInserter->bufferLength() > PACK_IN_MEM_LIMIT) {
						slept++;
						usleep(1000);
					}
				}
				// reset count to 0
				count = 0;
			}
		}
		delete readers[i];
	}
	mProduceFinished = true;
	if (mOptions->verbose) {
		loginfo("all reads loaded, start to monitor thread status");
	}
	if (data != NULL)
		delete[] data;
}

void BarcodeToPositionMultiProtein::consumerTask(ProteinResult* result) {
	while (true) {
		while (mRepo.writePos <= mRepo.readPos) {
			if (mProduceFinished)
				break;
			usleep(1000);
		}
		if (mProduceFinished && mRepo.writePos == mRepo.readPos) {
			mFinishedThreads++;
			if (mOptions->verbose) {
				string msg = "finished " + to_string(mFinishedThreads) + " threads. Data processing completed.";
				loginfo(msg);
			}
			break;
		}
		if (mProduceFinished) {
			if (mOptions->verbose) {
				string msg = "thread is processing the " + to_string(mRepo.readPos) + "/" + to_string(mRepo.writePos) + " pack";
				loginfo(msg);
			}
			consumePack(result);
		}
		else {
			consumePack(result);
		}
	}

	if (mFinishedThreads == mOptions->thread) {
		if (mapInserter)
			mapInserter->setInputCompleted();
		if (mUnmappedWriter1)
			mUnmappedWriter1->setInputCompleted();
		if (mUnmappedWriter2)
			mUnmappedWriter2->setInputCompleted();
	}
	if (mOptions->verbose) {
		string msg = "finished one thread";
		loginfo(msg);
	}
}


void BarcodeToPositionMultiProtein::writeTask(WriterThread* config) {
	while (true) {
		if (config->isCompleted()) {
			config->output();
			break;
		}
		config->output();
	}

	if (mOptions->verbose) {
		string msg = config->getFilename() + " writer finished";
		loginfo(msg);
	}
}

void BarcodeToPositionMultiProtein::mapInsertTask(MapInsertThread* config){
	while (true) {
		if (config->isCompleted()){
			config->output();
			break;
		}
		config->output();
	}
	if (mOptions->verbose){
		string msg = "map insert finished";
		loginfo(msg);
	}
}
