#ifndef COMMON_H
#define COMMON_H

#define FASTP_VER "0.20.0"

#define _DEBUG false

#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>

typedef long long int int64;
typedef unsigned long long int uint64;

typedef int int32;
typedef unsigned int uint32;

typedef short int16;
typedef unsigned short uint16;

typedef char int8;
typedef unsigned char uint8;

const char ATCG_BASES[4] = {'A', 'C', 'T', 'G'};
const uint8 RC_BASE[4] = { 2, 3, 0, 1 };

static const int SEQ_BINARY_SIZE = 8;
static const int COUNT_BINARY_SUZE = 2;

static const long long int MAX_BARCODE = 0xffffffffffffffff;

#pragma pack(2) 


#pragma pack() 

// the limit of the queue to store the packs
// error may happen if it generates more packs than this number
static const int PACK_NUM_LIMIT  = 10000000;

//buckets number for barcode unordered set
static const int BARCODE_SET_LIMIT = 100000000;

// how many reads one pack has
static const int PACK_SIZE = 1000;

// if one pack is produced, but not consumed, it will be kept in the memory
// this number limit the number of in memory packs
// if the number of in memory packs is full, the producer thread should sleep
static const int PACK_IN_MEM_LIMIT = 500;

// if read number is more than this, warn it
static const int WARN_STANDALONE_READ_LIMIT = 10000;

//block number per fov
static const int BLOCK_COL_NUM = 10;
static const int BLOCK_ROW_NUM = 10;
static const int TRACK_WIDTH = 3;
static const int FOV_GAP = 0;
static const int MAX_DNB_EXP = 250;

static const int EST_DNB_DISTANCE = 1;

//outside dnb idx reture value
static const int OUTSIDE_DNB_POS_ROW = 1410065408;
static const int OUTSIDE_DNB_POS_COL = 1410065408;

typedef struct slideRange{
	uint32 colStart;
	uint32 colEnd;
	uint32 rowStart;
	uint32 rowEnd;
}slideRange;

typedef struct Position
{
	friend class boost::serialization::access;
	uint8 fov_c;
	uint8 fov_r;
	uint32 x;
	uint32 y;
	
	template <typename Archive> 
	void serialize(Archive &ar, const unsigned int version){
		ar & fov_c ;
		ar & fov_r ;
		ar & x ;
		ar & y ;
	}
}Position;

typedef struct Position1{
	friend class boost::serialization::access;
	uint32 x;
	uint32 y;

	template <typename Archive>
	void serialize(Archive &ar, const unsigned int version){
		ar & x;
		ar & y;
	}
}Position1;

#endif /* COMMON_H */
