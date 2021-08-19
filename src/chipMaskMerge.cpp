#include "chipMaskMerge.h"

ChipMaskMerge::ChipMaskMerge(Options* opt){
    mOptions = opt;
    split(opt->in, inMasks, ",");
    outMask = opt->out;
}

ChipMaskMerge::~ChipMaskMerge(){
    unordered_map<uint64, Position1>().swap(bpmap);
}

void ChipMaskMerge::maskMerge(){
    time_t start = time(NULL);
	cout << "##########load barcodeToPosition map begin..." << endl;
    for (auto inMask = inMasks.begin(); inMask != inMasks.end(); inMask++){
        if (ends_with(*inMask, ".bin")){
            ifstream mapReader(inMask->c_str(), ios::in | ios::binary);
            uint64 barcodeInt;
		    Position1 position;
		    while (!mapReader.eof()) {
			    mapReader.read((char*)&barcodeInt, sizeof(barcodeInt));
			    mapReader.read((char*)&position.x, sizeof(position.x));
			    mapReader.read((char*)&position.y, sizeof(position.y));
                rangeRefresh(position);
                add(barcodeInt, position);
            }
            cout << "slide range: " << minX << "\t" << maxX << "\t" << minY << "\t" << maxY << endl;
        }else if (ends_with(*inMask, ".h5")){
            hid_t fileID = H5Fopen(inMask->c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            herr_t status;
            //open dataset with datasetName
            int index = 1;
            std::string datasetName = DATASETNAME + std::to_string(index);
            hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);
            //read attribute of the dataset
    
            uint32 attributeValues[ATTRIBUTEDIM];
            hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
            status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
            uint32 rowOffset = attributeValues[0];
            uint32 colOffset = attributeValues[1];
            barcodeLen = attributeValues[2];
            cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

            hid_t dspaceID = H5Dget_space(datasetID);
            hid_t dtype_id = H5Dget_type(datasetID);
            hid_t plistID = H5Dget_create_plist(datasetID);
            int rank = H5Sget_simple_extent_ndims(dspaceID);
            hsize_t dims[rank];
            status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

            uint64 matrixLen = 1;
            for (int i = 0 ; i<rank; i++){
                matrixLen *= dims[i];
            }

            int segment = 1;
            if (rank>=3){
                segment = dims[2];
            }

            uint64* bpMatrix_buffer = new uint64[matrixLen]();
            status = H5Dread(datasetID, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, bpMatrix_buffer);
            status = H5Aclose(attributeID);
            status = H5Dclose(datasetID);
            status = H5Fclose(fileID);
    
    
            for (uint32 r = 0; r < dims[0]; r++){
                
                for (uint32 c = 0; c< dims[1]; c++){
                    Position1 position = {c + colOffset, r + rowOffset};
                    if (rank >= 3 ){               
                        segment = dims[2];
                        for (int s = 0; s<segment; s++){
                            uint64 barcodeInt = bpMatrix_buffer[r*dims[1]*segment + c*segment + s];
                            if (barcodeInt == 0){
                                continue;
                            }
                            add(barcodeInt, position);
                        }
                    }else{
                        uint64 barcodeInt = bpMatrix_buffer[r*dims[1]+c];
                        if (barcodeInt == 0){
                            continue;
                        }
                        add(barcodeInt, position);
                    }           
                }
            }
            Position1 positionR = {dims[1] + colOffset, dims[0] + rowOffset};
            rangeRefresh(positionR);
            Position1 positionL = {colOffset, rowOffset};
            rangeRefresh(positionL);
        }else{
            uint64 barcodeInt;
		    Position1 position;
		    string line;
		    ifstream mapReader(inMask->c_str());
		    while (std::getline(mapReader, line)) {
			    if(line.empty()){
				    cerr << "barcodePositionMap file read finished." << endl;
				    break;
			    }
			    vector<string> splitLine;
			    //std::getline(mapReader, line);
			    split(line, splitLine, "\t");
			    //position.fov_c = fovCol;
			    //position.fov_r = fovRow;
			    if (splitLine.size() < 3){
				    break;
			    }
			    else if (splitLine.size() == 3){
				    position.x = std::stoi(splitLine[1]);
				    position.y = std::stoi(splitLine[2]);
			    }else {
				    position.x = std::stoi(splitLine[3]);
				    position.y = std::stoi(splitLine[4]);
			    }
			    barcodeInt = seqEncode(splitLine[0].c_str(), mOptions->barcodeStart, barcodeLen);
                rangeRefresh(position);
			    add(barcodeInt, position);
			    //cout << "barcode: " << barcodeInt << " position: " << position.x << " " << position.y <<endl;
		    }
		    cout << "slide range: " << minX << "\t" << maxX << "\t" << minY << "\t" << maxY << endl;
		    mapReader.close();
        }
    }
    long uniqueBarcodes = bpmap.size();
    cout << "##########load barcodeToPosition map finished, time used: " << time(NULL) - start << " seconds" << endl;
    cout << "total barcode number:\t" << totalBarcodes << endl;
    cout << "overlaped barcode number:\t" << overlapBarcodes << endl;
    cout << "duplicated barcode number:\t" << dupBarcodes << endl;
    cout << "merged unique barcode number:\t" << uniqueBarcodes << endl;
    if (ends_with(mOptions->report, ".html")){
        HtmlReporter htmlReport(mOptions);
        htmlReport.printReport(mOptions->report, totalBarcodes, overlapBarcodes, dupBarcodes, uniqueBarcodes, inMasks);
    }
    dumpBpmap();
}

bool ChipMaskMerge::add(uint64 barcodeInt, Position1& position){
    totalBarcodes++;
    if (dupBarcode.count(barcodeInt) > 0){
        dupBarcodes++;
        return false;
    }
    unordered_map<uint64, Position1>::iterator dupBarcodeFind = bpmap.find(barcodeInt);
    if (dupBarcodeFind != bpmap.end()){
        if (dupBarcodeFind->second.x == position.x && dupBarcodeFind->second.y == position.y){
            overlapBarcodes++;
            return false;
        }else{
            dupBarcodes += 2;
            dupBarcode.insert(barcodeInt);
            bpmap.erase(barcodeInt);
            return false;
        }
    }else{
        bpmap[barcodeInt] = position;
        return true;
    }
}

void ChipMaskMerge::rangeRefresh(Position1& position){
	if (position.x < minX){
		minX  = position.x;
	}else if (position.x > maxX){
		maxX = position.x;
	}
	if (position.y < minY){
		minY = position.y;
	}else if (position.y > maxY){
		maxY = position.y;
	}
}

void ChipMaskMerge::dumpBpmap(){
    time_t start = time(NULL);
	cout << "##########dump barcodeToPosition map begin..." << endl;
	if (ends_with(outMask, ".bin")) {
		unordered_map<uint64, Position1>::iterator mapIter = bpmap.begin();
		bpmap.reserve(bpmap.size());
		ofstream writer(outMask, ios::out | ios::binary);
		//boost::archive::binary_oarchive oa(writer);
		//oa << bpmap;
		while (mapIter != bpmap.end()) {
			writer.write((char*)&mapIter->first, sizeof(uint64));
			writer.write((char*)&mapIter->second, sizeof(uint32));
			writer.write((char*)&mapIter->second, sizeof(uint32));
			mapIter++;
		}
		writer.close();
	}else if (ends_with(outMask, "h5") || ends_with(outMask, "hdf5")){
		ChipMaskHDF5 chipMaskH5(outMask);
		chipMaskH5.creatFile();
		uint8_t segment = mOptions->barcodeSegment;
		if (mOptions->rc == 2){
			segment *= 2;
		}
        slideRange sliderange{minX, maxX, minY, maxY};
		chipMaskH5.writeDataSet(mOptions->chipID, sliderange, bpmap, barcodeLen, segment, slidePitch, mOptions->compression);
	}
	else {
		ofstream writer(outMask);
		unordered_map<uint64, Position1>::iterator mapIter = bpmap.begin();
		while (mapIter != bpmap.end()) {
			writer << seqDecode(mapIter->first, barcodeLen) << "\t" << mapIter->second.x << "\t" << mapIter->second.y << endl;
			mapIter++;
		}
		writer.close();
	}
	cout << "##########dump barcodeToPosition map finished, time used: " << time(NULL) - start << " seconds" << endl;
}
