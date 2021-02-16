# ST_Barcode_map
This program can map barcode of stereomics-seq to stereomics-chip

## Compile
### Platform& Environment
* linux
* gcc-8.2.0

## Prerequisties
| Package       | Version  | Description                                                |
| ------------- | -------- | ---------------------------------------------------------- |
| boost         | >=1.73.0 | serialize hashmap                                          |
| zlib          | >=1.2.11 | file compressing and decompressing                         |

## Run
### Usage
```
usage: ST_BarcodeMap-0.0.1 --in=string --out=string [options] ... 
options:
  -i, --in                   the first sequencing fastq file path of read or bin file of first sequencing (string)
  -I, --in1                  the second sequencing fastq file path of read1 or bin file of second sequencing (string [=])
      --in2                  the second sequencing fastq file or bin file path of read1 (string [=])
      --barcodeReadsCount    the mapped barcode list file with reads count per barcode. (string [=])
  -O, --out                  output file prefix or fastq output file of read1 (string)
      --out2                 fastq output file of read2 (string [=])
      --report               logging file path. (string [=])
      --PEout                if this option was given, PE reads with barcode tag will be writen
  -z, --compression          compression level for gzip output (1 ~ 9). 1 is fastest, 9 is smallest, default is 4. (int [=4])
      --unmappedOut          output file path for barcode unmapped reads of read1, if this path isn't given, discard the reads. (string [=])
      --unmappedOut2         output file path for barcode unmapped reads of read2, if this path isn't given, discard the reads. (string [=])
  -l, --barcodeLen           barcode length, default  is 25 (int [=25])
      --barcodeStart         barcode start position (int [=0])
      --umiRead              read1 or read2 contains the umi sequence. (int [=1])
      --barcodeRead          read1 or read2 contains the barcode sequence. (int [=1])
      --umiStart             umi start position. if the start postion is negative number, no umi sequence will be found (int [=40])
      --umiLen               umi length. (int [=10])
      --fixedSequence        fixed sequence in read1 that will be filtered. (string [=])
      --fixedStart           fixed sequence start position can by specied. (int [=-1])
      --fixedSequenceFile    file contianing the fixed sequences and the start position, one sequence per line in the format: TGCCTCTCAG        -1. when position less than 0, means wouldn't specified (string [=])
      --mapSize              bucket size of the new unordered_map. (long [=0])
      --mismatch             max mismatch is allowed for barcode overlap find. (int [=0])
      --rc                   true means get the reverse complement barcode to barcode map. all means get both forward and reverse complement barcode to barcode map (string [=false])
      --readidSep            number of characters will be trimed from readid to get read position in slide. If not given this will be automatically get from fastq file. (string [=/])
      --action               chose one action you want to run [map_barcode_to_slide = 1, merge_barcode_list = 2]. (int [=1])
  -w, --thread               number of thread that will be used to run. (int [=2])
  -V, --verbose              output verbose log information (i.e. when every 1M reads are processed).
  -?, --help                 print this message
  ```