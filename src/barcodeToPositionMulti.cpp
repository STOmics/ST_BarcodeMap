#include "barcodeToPositionMulti.h"

BarcodeToPositionMulti::BarcodeToPositionMulti(Options* opt)
{
	mOptions = opt;
	mProduceFinished = false;
	mFinishedThreads = 0;
	mOutStream = NULL;
	mZipFile = NULL;
	mWriter = NULL;
	mUnmappedWriter = NULL;
	bool isSeq500 = opt->isSeq500;
	mbpmap = new BarcodePositionMap(opt);
	//barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
	if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
		fixedFilter = new FixedFilter(opt);
		filterFixedSequence = true;
	}
}

BarcodeToPositionMulti::~BarcodeToPositionMulti()
{
	if (fixedFilter) {
		delete fixedFilter;
	}
	//unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMulti::process()
{
	initOutput();
	initPackRepositoey();
	std::thread producer(std::bind(&BarcodeToPositionMulti::producerTask, this));

	Result** results = new Result*[mOptions->thread];
	BarcodeProcessor** barcodeProcessors = new BarcodeProcessor*[mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		results[t] = new Result(mOptions, true);
		results[t]->setBarcodeProcessor(mbpmap->getBpmap());
	}

	std::thread** threads = new thread * [mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t] = new std::thread(std::bind(&BarcodeToPositionMulti::consumerTask, this, results[t]));
	}

	std::thread* writerThread = NULL;
	std::thread* unMappedWriterThread = NULL;
	if(mWriter){
		writerThread = new std::thread(std::bind(&BarcodeToPositionMulti::writeTask, this, mWriter));
	}
	if (mUnmappedWriter) {
		unMappedWriterThread = new std::thread(std::bind(&BarcodeToPositionMulti::writeTask, this, mUnmappedWriter));
	}

	producer.join();
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t]->join();
	}

	if (writerThread)
		writerThread->join();
	if (unMappedWriterThread)
		unMappedWriterThread->join();

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

	if (writerThread)
		delete writerThread;
	if (unMappedWriterThread)
		delete unMappedWriterThread;

	closeOutput();

	return true;
}

void BarcodeToPositionMulti::initOutput() {
	mWriter = new WriterThread(mOptions->out, mOptions->compression);
	if (!mOptions->transBarcodeToPos.unmappedOutFile.empty()) {
		mUnmappedWriter = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile, mOptions->compression);
	}
}

void BarcodeToPositionMulti::closeOutput()
{
	if (mWriter) {
		delete mWriter;
		mWriter = NULL;
	}
	if (mUnmappedWriter) {
		delete mUnmappedWriter;
		mUnmappedWriter = NULL;
	}
}

bool BarcodeToPositionMulti::processPairEnd(ReadPairPack* pack, Result* result)
{
	string outstr;
	string unmappedOut;
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
			outstr += or2->toString();
		}
		else if (mUnmappedWriter) {
			unmappedOut += or2->toString();
		}
		delete pair;
	}
	mOutputMtx.lock();
	if (mUnmappedWriter && !unmappedOut.empty()) {
		//write reads that can't be mapped to the slide
		char* udata = new char[unmappedOut.size()];
		memcpy(udata, unmappedOut.c_str(), unmappedOut.size());
		mUnmappedWriter->input(udata, unmappedOut.size());
	}
	if (mWriter && !outstr.empty()) {
		char* data = new char[outstr.size()];
		memcpy(data, outstr.c_str(), outstr.size());
		mWriter->input(data, outstr.size());
	}
	mOutputMtx.unlock();
	delete pack->data;
	delete pack;
	return true;
}

void BarcodeToPositionMulti::initPackRepositoey()
{
	mRepo.packBuffer = new ReadPairPack * [PACK_NUM_LIMIT];
	memset(mRepo.packBuffer, 0, sizeof(ReadPairPack*) * PACK_NUM_LIMIT);
	mRepo.writePos = 0;
	mRepo.readPos = 0;
}

void BarcodeToPositionMulti::destroyPackRepository() {
	delete mRepo.packBuffer;
	mRepo.packBuffer = NULL;
}

void BarcodeToPositionMulti::producePack(ReadPairPack* pack) {
	mRepo.packBuffer[mRepo.writePos] = pack;
	mRepo.writePos++;
}

void BarcodeToPositionMulti::consumePack(Result* result) {
	ReadPairPack* data;
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

void BarcodeToPositionMulti::producerTask() {
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
			ReadPairPack* pack = new ReadPairPack;
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
			ReadPairPack* pack = new ReadPairPack;
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
			if (readNum % (PACK_SIZE * PACK_IN_MEM_LIMIT) == 0 && mWriter) {
				while ((mWriter && mWriter->bufferLength() > PACK_IN_MEM_LIMIT)) {
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

void BarcodeToPositionMulti::consumerTask(Result* result) {
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
		if (mWriter)
			mWriter->setInputCompleted();
		if (mUnmappedWriter)
			mUnmappedWriter->setInputCompleted();
	}
	if (mOptions->verbose) {
		string msg = "finished one thread";
		loginfo(msg);
	}
}

void BarcodeToPositionMulti::writeTask(WriterThread* config) {
	while (true) {
		//loginfo("writeTask running: " + config->getFilename());
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
