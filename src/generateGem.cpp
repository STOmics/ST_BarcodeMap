#include "generateGem.h"

GenerateGem::GenerateGem(unordered_map<uint64, unordered_map<uint64, uint32>>* GemMap, int umiLength){
    gemMap = GemMap;
    umiLen = umiLength;
}

GenerateGem::~GenerateGem(){

}
void GenerateGem::umiCorrection(){
    vector< pair<uint64, uint32> > array;
    for (auto p = gemMap->begin(); p!=gemMap->end(); p++){
        if (p->second.size() < numThre){
            continue;
        }
        array.clear();
        for (auto umi = p->second.begin(); umi!=p->second.end(); umi++){
            array.push_back({umi->first, umi->second});
        }
        sort(array.begin(), array.end(), compareBySecond);
        for (size_t i = array.size() - 1; i > 0; --i)
        {
            for (size_t j = 0; j < i; ++j)
            {
                // Calculate distance of two umis
                int distance = 0;
                distance = umiDistance(array[i].first, array[j].first);
                if (distance <= mismatch)
                {
                    p->second[array[j].first] += p->second[array[i].first];
                    p->second.erase(array[i].first);
                    // Break current umi
                    break;
                }
            }
        }
    }
}

void GenerateGem::dumpGem(string rawGemFile, string gemFile, unordered_map<uint16, string>& proteinNameMap){
    ofstream rawGemWriter(rawGemFile);
    ofstream gemWriter(gemFile);
    gemWriter << "geneID\tx\ty\tMIDCount\n" ;
    uint32 x;
    uint32 y;
    uint16 proteinIndex;
    string proteinName;
    long uniqueReads = 0;
    long totalReads = 0;
    int umiCount = 0;
    for (auto iter = gemMap->begin(); iter != gemMap->end(); iter++){
        decodeCIDGene(iter->first, x, y, proteinIndex);
        proteinName = proteinNameMap[proteinIndex];
        umiCount = 0;        
        for (auto umi = iter->second.begin(); umi != iter->second.end(); umi++){
            rawGemWriter << y << "\t" << x << "\t" << proteinIndex << "\t" << umi->first << "\t" << umi->second << endl;
            umiCount++;
            totalReads+=umi->second;
        }
        gemWriter << proteinName << "\t" << x << "\t" << y << "\t" <<  umiCount << endl;
        uniqueReads+=umiCount;
    }
    double duplicationRate = (1 - ((double)uniqueReads/(double)totalReads))*100;
    cout << fixed << setprecision(2);
    cout << "mapped_proteinBarcode_uniqueReads:\t" << uniqueReads << endl;
    cout << "duplication_rate:\t" << duplicationRate << "%" << endl;;
}

int GenerateGem::umiDistance(uint64 umi1, uint64 umi2){
    string s1 = seqDecode(umi1, umiLen);
    string s2 = seqDecode(umi2, umiLen);
    int distance = 0;
    for (size_t i = 0; i < s1.size(); ++i)
    {
        if (s1[i] != s2[i])
        {
            ++distance;
        }
    }
    return distance;
}
