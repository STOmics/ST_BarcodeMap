#include "proteinResult.h"

ProteinResult::ProteinResult(Options* opt, int threadId, bool paired)
{
	mOptions = opt;
	mPaired = paired;
	mTotalRead = 0;
	mFxiedFilterRead = 0;
	mWithoutPositionReads = 0;
	mLowQuaRead = 0;
	overlapReadsWithMis = 0;
	overlapReadsWithN = 0;

	mThreadId = threadId;
	if (opt->verbose) {
		string msg = "new result class in thread: " + to_string(mThreadId);
		loginfo(msg);
	}
}

ProteinResult::~ProteinResult()
{
	delete mBarcodeProcessor;
}

ProteinResult* ProteinResult::merge(vector<ProteinResult*>& list)
{
	if (list.size() == 0)
		return nullptr;
	ProteinResult* result = new ProteinResult(list[0]->mOptions, 0, list[0]->mPaired);
	result->setBarcodeProcessor();

	for (int i = 0; i < list.size(); i++) {
		result->mTotalRead += list[i]->mTotalRead;
		result->mFxiedFilterRead += list[i]->mFxiedFilterRead;
		result->mWithoutPositionReads += list[i]->mWithoutPositionReads;
		result->mLowQuaRead += list[i]->mLowQuaRead;
		result->mBarcodeProcessor->totalReads += list[i]->mBarcodeProcessor->totalReads;
		result->mBarcodeProcessor->mMapToSlideRead += list[i]->mBarcodeProcessor->mMapToSlideRead;
		result->mBarcodeProcessor->overlapReads += list[i]->mBarcodeProcessor->overlapReads;
		result->mBarcodeProcessor->overlapReadsWithMis += list[i]->mBarcodeProcessor->overlapReadsWithMis;
		result->mBarcodeProcessor->overlapReadsWithN += list[i]->mBarcodeProcessor->overlapReadsWithN;
		result->mBarcodeProcessor->barcodeQ10 += list[i]->mBarcodeProcessor->barcodeQ10;
		result->mBarcodeProcessor->barcodeQ20 += list[i]->mBarcodeProcessor->barcodeQ20;
		result->mBarcodeProcessor->barcodeQ30 += list[i]->mBarcodeProcessor->barcodeQ30;
		result->mBarcodeProcessor->umiQ10 += list[i]->mBarcodeProcessor->umiQ10;
		result->mBarcodeProcessor->umiQ20 += list[i]->mBarcodeProcessor->umiQ20;
		result->mBarcodeProcessor->umiQ30 += list[i]->mBarcodeProcessor->umiQ30;
		result->mBarcodeProcessor->seqQ10 += list[i]->mBarcodeProcessor->seqQ10;
		result->mBarcodeProcessor->seqQ20 += list[i]->mBarcodeProcessor->seqQ20;
		result->mBarcodeProcessor->seqQ30 += list[i]->mBarcodeProcessor->seqQ30;
		result->mBarcodeProcessor->umiNFilterReads += list[i]->mBarcodeProcessor->umiNFilterReads;
		result->mBarcodeProcessor->umiPloyAFilterReads += list[i]->mBarcodeProcessor->umiPloyAFilterReads;
		result->mBarcodeProcessor->umiQ10FilterReads += list[i]->mBarcodeProcessor->umiQ10FilterReads;
		result->mBarcodeProcessor->readsMatchProtein += list[i]->mBarcodeProcessor->readsMatchProtein;	
	}
	return result;
}

void ProteinResult::print()
{
	
	cout << fixed << setprecision(2);
	cout << "total_reads:\t" << mTotalRead << endl;
	cout << "fixed_sequence_contianing_reads:\t" << mFxiedFilterRead << "\t" << (double)mFxiedFilterRead / (double)mTotalRead * 100 << "%" << endl;
	cout << "pass_filter_reads:\t" << mBarcodeProcessor->totalReads << endl;
	cout << "mapped_reads:\t" << mBarcodeProcessor->mMapToSlideRead << "\t" << (double)mBarcodeProcessor->mMapToSlideRead / (double)(mTotalRead - mFxiedFilterRead)* 100 << "%" << endl;
	double overlapReadsWithMisRate = (double)mBarcodeProcessor->overlapReadsWithMis / (double)mBarcodeProcessor->totalReads * 100;
	double overlapReadsWithNRate = (double)mBarcodeProcessor->overlapReadsWithN / (double)mBarcodeProcessor->totalReads * 100;
	cout << "barcode_exactlyOverlap_reads:\t" << mBarcodeProcessor->overlapReads << "\t" << (double)mBarcodeProcessor->overlapReads / (double)mBarcodeProcessor->totalReads * 100 << "%" << endl;
	cout << "barcode_misOverlap_reads:\t" << mBarcodeProcessor->overlapReadsWithMis << "\t" << overlapReadsWithMisRate << "%" << endl;
	cout << "barcode_withN_reads:\t" << mBarcodeProcessor->overlapReadsWithN << "\t" << overlapReadsWithNRate << "%" << endl;
	double barcodeQ10 = (double)mBarcodeProcessor->barcodeQ10 / (double)(mBarcodeProcessor->totalReads * mOptions->barcodeLen) * 100;
	double barcodeQ20 = (double)mBarcodeProcessor->barcodeQ20 / (double)(mBarcodeProcessor->totalReads * mOptions->barcodeLen) * 100;
	double barcodeQ30 = (double)mBarcodeProcessor->barcodeQ30 / (double)(mBarcodeProcessor->totalReads * mOptions->barcodeLen) * 100;
	double umiQ10 = 0;
	double umiQ20 = 0;
	double umiQ30 = 0;
	double seqQ10 = 0;
	double seqQ20 = 0;
	double seqQ30 = 0;
	if (mOptions->transBarcodeToPos.umiLen>0){
		umiQ10 = (double)mBarcodeProcessor->umiQ10 / (double)(mBarcodeProcessor->mMapToSlideRead * mOptions->transBarcodeToPos.umiLen) * 100;
		umiQ20 = (double)mBarcodeProcessor->umiQ20 / (double)(mBarcodeProcessor->mMapToSlideRead * mOptions->transBarcodeToPos.umiLen) * 100;
		umiQ30 = (double)mBarcodeProcessor->umiQ30 / (double)(mBarcodeProcessor->mMapToSlideRead * mOptions->transBarcodeToPos.umiLen) * 100;
	}
	if (mOptions->transBarcodeToPos.protein){
		seqQ10 = (double)mBarcodeProcessor->seqQ10 / (double(mBarcodeProcessor->mMapToSlideRead * mOptions->transBarcodeToPos.proteinBarcodeLen)) * 100;
		seqQ20 = (double)mBarcodeProcessor->seqQ20 / (double(mBarcodeProcessor->mMapToSlideRead * mOptions->transBarcodeToPos.proteinBarcodeLen)) * 100;
		seqQ30 = (double)mBarcodeProcessor->seqQ30 / (double(mBarcodeProcessor->mMapToSlideRead * mOptions->transBarcodeToPos.proteinBarcodeLen)) * 100;
	}
	cout << "Q10_bases_in_barcode:\t" << barcodeQ10 << "%" << endl;
	cout << "Q20_bases_in_barcode:\t" << barcodeQ20 << "%" << endl;
	cout << "Q30_bases_in_barcode:\t" << barcodeQ30 << "%" << endl;
	cout << "Q10_bases_in_umi:\t" << umiQ10 << "%" << endl;
	cout << "Q20_bases_in_umi:\t" << umiQ20 << "%" << endl;
	cout << "Q30_bases_in_umi:\t" << umiQ30 << "%" << endl;
	if (mOptions->transBarcodeToPos.protein){
		cout << "Q10_bases_in_reads:\t" << seqQ10 << "%" << endl;
		cout << "Q20_bases_in_reads:\t" << seqQ20 << "%" << endl;
		cout << "Q30_bases_in_reads:\t" << seqQ30 << "%" << endl;
	}
	long umiTotalFilterReads = mBarcodeProcessor->umiNFilterReads + mBarcodeProcessor->umiPloyAFilterReads + mBarcodeProcessor->umiQ10FilterReads;
	double umiFilterRate = (double)umiTotalFilterReads/(double)mBarcodeProcessor->totalReads * 100;
	double umiNFilterRate = (double)mBarcodeProcessor->umiNFilterReads/(double)mBarcodeProcessor->totalReads * 100;
	double umiPolyAFilterRate = (double)mBarcodeProcessor->umiPloyAFilterReads/(double)mBarcodeProcessor->totalReads * 100;
	double umiQ10FilterRate = (double)mBarcodeProcessor->umiQ10FilterReads/(double)mBarcodeProcessor->totalReads * 100;
	double readsMatchProteinRate = (double)mBarcodeProcessor->readsMatchProtein/(double(mBarcodeProcessor->mMapToSlideRead - umiTotalFilterReads)) * 100;
	cout << "umi_filter_reads:\t" << umiTotalFilterReads << "\t" << umiFilterRate << "%" << endl;
	cout << "umi_with_N_reads:\t" << mBarcodeProcessor->umiNFilterReads << "\t" << umiNFilterRate << "%" << endl;
	cout << "umi_with_polyA_reads:\t" << mBarcodeProcessor->umiPloyAFilterReads << "\t" << umiPolyAFilterRate << "%" << endl;
	cout << "umi_with_low_quality_base_reads:\t" << mBarcodeProcessor->umiQ10FilterReads << "\t" << umiQ10FilterRate << "%" << endl;
	cout << "mapped_proteinBarcode_reads:\t" << mBarcodeProcessor->readsMatchProtein << "\t" << readsMatchProteinRate  << "%" << endl;
}

void ProteinResult::setBarcodeProcessor(unordered_map<uint64, Position1>* bpmap, unordered_map<uint64, uint16>* barcodePositionMap)
{
	mBarcodeProcessor = new ProteinBarcodeProcessor(mOptions, bpmap, barcodePositionMap);
}

void ProteinResult::setBarcodeProcessor()
{
	mBarcodeProcessor = new ProteinBarcodeProcessor();
}



