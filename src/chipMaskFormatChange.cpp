#include "chipMaskFormatChange.h"

ChipMaskFormatChange::ChipMaskFormatChange(Options* opt){
    mOptions = opt;
}

ChipMaskFormatChange::~ChipMaskFormatChange(){

}

void ChipMaskFormatChange::change(){
	bpmap = new BarcodePositionMap(mOptions);
	bpmap->dumpbpmap(mOptions->out);
}

void ChipMaskFormatChange::H5ToBin(){
	hid_t fileID = H5Fopen(mOptions->in.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
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
    
	ofstream writer(mOptions->out, ios::out | ios::binary);
    
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
                    writer.write((char*)&barcodeInt, sizeof(barcodeInt));
					writer.write((char*)&position.x, sizeof(position.x));
					writer.write((char*)&position.y, sizeof(position.y));
                }
            }else{
                uint64 barcodeInt = bpMatrix_buffer[r*dims[1]+c];
                if (barcodeInt == 0){
                    continue;
                }
                writer.write((char*)&barcodeInt, sizeof(barcodeInt));
				writer.write((char*)&position.x, sizeof(position.x));
				writer.write((char*)&position.y, sizeof(position.y));
            }           
        }
    }
    writer.close();  
}

