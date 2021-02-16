#ifndef WRITER_THREAD_H
#define WRITER_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include "writer.h"
#include <atomic>
#include <mutex>

using namespace std;

class WriterThread {
public:
	WriterThread(string filename, int compressionLevel = 4);
	~WriterThread();

	void initWriter(string filename1);
	void initWriter(ofstream* stream);
	void initWriter(gzFile gzfile);

	void cleanup();

	bool isCompleted();
	void output();
	void input(char* data, size_t size);
	bool setInputCompleted();

	long bufferLength();
	string getFilename() { return mFilename; }

private:
	void deleteWriter();

private:
	Writer* mWriter1;
	int compression;
	string mFilename;

	//for split output
	bool mInputCompleted;
	atomic_long mInputCounter;
	atomic_long mOutputCounter;
	char** mRingBuffer;
	size_t* mRingBufferSizes;

	mutex mtx;
};

#endif