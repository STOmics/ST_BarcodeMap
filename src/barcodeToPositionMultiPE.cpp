#include "barcodeToPositionMultiPE.h"

BarcodeToPositionMultiPE::BarcodeToPositionMultiPE(Options* opt)
{
	mOptions = opt;
	mProduceFinished = false;
	mFinishedThreads = 0;
	mOutStream = NULL;
	mZipFile = NULL;
	mWriter1 = NULL;
	mWriter2 = NULL;
	mUnmappedWriter1 = NULL;
	mUnmappedWriter2 = NULL;
	bool isSeq500 = opt->isSeq500;
	mbpmap = new BarcodePositionMap(opt);
	//barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
	if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
		fixedFilter = new FixedFilter(opt);
		filterFixedSequence = true;
	}
}

BarcodeToPositionMultiPE::~BarcodeToPositionMultiPE()
{
	if (fixedFilter) {
		delete fixedFilter;
	}
	//unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMultiPE::process()
{
	initOutput();
	initPackRepositoey();
	std::thread producer(std::bind(&BarcodeToPositionMultiPE::producerTask, this));

	Result** results = new Result*[mOptions->thread];
	BarcodeProcessor** barcodeProcessors = new BarcodeProcessor*[mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		results[t] = new Result(mOptions, true);
		results[t]->setBarcodeProcessor(mbpmap->getBpmap());
	}

	std::thread** threads = new thread * [mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t] = new std::thread(std::bind(&BarcodeToPositionMultiPE::consumerTask, this, results[t]));
	}

	std::thread* writerThread1 = NULL;
	std::thread* writerThread2 = NULL;
	std::thread* unMappedWriterThread1 = NULL;
	std::thread* unMappedWriterThread2 = NULL;
	if(mWriter1 && mWriter2){
		writerThread1 = new std::thread(std::bind(&BarcodeToPositionMultiPE::writeTask, this, mWriter1));
		writerThread2 = new std::thread(std::bind(&BarcodeToPositionMultiPE::writeTask, this, mWriter2));
	}
	if (mUnmappedWriter1 && mUnmappedWriter2) {
		unMappedWriterThread1 = new std::thread(std::bind(&BarcodeToPositionMultiPE::writeTask, this, mUnmappedWriter1));
		unMappedWriterThread2 = new std::thread(std::bind(&BarcodeToPositionMultiPE::writeTask, this, mUnmappedWriter2));
		
	}

	producer.join();
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t]->join();
	}

	if (writerThread1)
		writerThread1->join();
	if (writerThread2)
		writerThread2->join();
	if (unMappedWriterThread1)
		unMappedWriterThread1->join();
	if (unMappedWriterThread2)
		unMappedWriterThread2->join();

	if (mOptions->verbose)
		loginfo("start to generate reports\n");

	//merge result
	vector<Result*> resultList;
	for (int t = 0; t < mOptions->thread; t++){
		resultList.push_back(results[t]);
	}
	Result* finalResult = Result::merge(resultList);
	finalResult->print();

	
	cout << resetiosflags(ios::fixed) << setprecision(2);
	if (!mOptions->transBarcodeToPos.mappedDNBOutFile.empty()) {
		cout << "mapped_dnbs: " << finalResult->mBarcodeProcessor->mDNB.size() << endl;
		finalResult->dumpDNBs(mOptions->transBarcodeToPos.mappedDNBOutFile);
	}
	
	//clean up
	for (int t = 0; t < mOptions->thread; t++) {
		delete threads[t];
		threads[t] = NULL;
		delete results[t];
		results[t] = NULL;
	}

	delete[] threads;
	delete[] results;

	if (writerThread1)
		delete writerThread1;
	if (writerThread2)
		delete writerThread2;
	if (unMappedWriterThread1)
		delete unMappedWriterThread1;
	if (unMappedWriterThread2)
		delete unMappedWriterThread2;

	closeOutput();

	return true;
}

void BarcodeToPositionMultiPE::initOutput() {
	mWriter1 = new WriterThread(mOptions->transBarcodeToPos.out1, mOptions->compression);
	mWriter2 = new WriterThread(mOptions->transBarcodeToPos.out2, mOptions->compression);
	if (!mOptions->transBarcodeToPos.unmappedOutFile.empty() && !mOptions->transBarcodeToPos.unmappedOutFile2.empty()) {
		mUnmappedWriter1 = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile, mOptions->compression);
		mUnmappedWriter2 = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile2, mOptions->compression);
	}
}

void BarcodeToPositionMultiPE::closeOutput()
{
	if (mWriter1) {
		delete mWriter1;
		mWriter1 = NULL;
	}
	if (mWriter2) {
		delete mWriter2;
		mWriter2 = NULL;
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

bool BarcodeToPositionMultiPE::processPairEnd(ReadPairPack1* pack, Result* result)
{
	string outstr1;
	string outstr2;
	string unmappedOut1;
	string unmappedOut2;
	bool hasPosition;
	bool fixedFiltered;
	for (int p = 0; p < pack->count; p++) {
		result->mTotalRead++;
		ReadPair* pair = pack->data[p];
		Read* or1 = pair->mLeft;
		Read* or2 = pair->mRight;
		if (filterFixedSequence) {
			fixedFiltered = fixedFilter->filter(or1, or2, result);
			if (fixedFiltered) {
				delete pair;
				continue;
			}
		}
		hasPosition = result->mBarcodeProcessor->process(or1, or2);
		if (hasPosition) {
			outstr1 += or1->toString();
			outstr2 += or2->toString();
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
	
	if (mWriter1 && mWriter2 && (!outstr1.empty() || !outstr2.empty())) {
		char* data1 = new char[outstr1.size()];
		memcpy(data1, outstr1.c_str(), outstr1.size());
		mWriter1->input(data1, outstr1.size());

		char* data2 = new char[outstr2.size()];
		memcpy(data2, outstr2.c_str(), outstr2.size());
		mWriter2->input(data2, outstr2.size());
	}
	mOutputMtx.unlock();
	delete pack->data;
	delete pack;
	return true;
}

void BarcodeToPositionMultiPE::initPackRepositoey()
{
	mRepo.packBuffer = new ReadPairPack1 * [PACK_NUM_LIMIT];
	memset(mRepo.packBuffer, 0, sizeof(ReadPairPack1*) * PACK_NUM_LIMIT);
	mRepo.writePos = 0;
	mRepo.readPos = 0;
}

void BarcodeToPositionMultiPE::destroyPackRepository() {
	delete mRepo.packBuffer;
	mRepo.packBuffer = NULL;
}

void BarcodeToPositionMultiPE::producePack(ReadPairPack1* pack) {
	mRepo.packBuffer[mRepo.writePos] = pack;
	mRepo.writePos++;
}

void BarcodeToPositionMultiPE::consumePack(Result* result) {
	ReadPairPack1* data;
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

void BarcodeToPositionMultiPE::producerTask() {
	if (mOptions->verbose)
		loginfo("start to load data");
	long lastReported = 0;
	int slept = 0;
	long readNum = 0;
	ReadPair** data = new ReadPair * [PACK_SIZE];
	memset(data, 0, sizeof(ReadPair*) * PACK_SIZE);
	FastqReaderPair reader(mOptions->transBarcodeToPos.in1, mOptions->transBarcodeToPos.in2, true);
	int count = 0;
	bool needToBreak = false;
	while (true) {
		ReadPair* read = reader.read();
		if (!read || needToBreak) {
			ReadPairPack1* pack = new ReadPairPack1;
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
			ReadPairPack1* pack = new ReadPairPack1;
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
			if (readNum % (PACK_SIZE * PACK_IN_MEM_LIMIT) == 0 && mWriter1) {
				while ((mWriter1 && mWriter1->bufferLength() > PACK_IN_MEM_LIMIT) || (mWriter2 && mWriter2->bufferLength() > PACK_IN_MEM_LIMIT)) {
					slept++;
					usleep(1000);
				}
			}
			// reset count to 0
			count = 0;
		}
	}
	mProduceFinished = true;
	if (mOptions->verbose) {
		loginfo("all reads loaded, start to monitor thread status");
	}
	if (data != NULL)
		delete[] data;
}

void BarcodeToPositionMultiPE::consumerTask(Result* result) {
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
		if (mWriter1)
			mWriter1->setInputCompleted();
		if (mWriter2)
			mWriter2->setInputCompleted();
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

void BarcodeToPositionMultiPE::writeTask(WriterThread* config) {
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
