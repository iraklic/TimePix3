#ifndef ENTANGLED_H
#define ENTANGLED_H

#include <TString.h>
#include <TStopwatch.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#define MAX_HITS  65536      // minimal value for sorting 65536
#define MAX_FILES 256
#define ENTRY_LOOP 150
#define MAX_DIFF    312.5//156.25//15600 //936        //ns
#define CUT_TOT         0
#define CUT_SIZE        1

// 30 x 30
#define X1_SIZE  30
#define X1_LOW   170
#define X1_HIGH  (X1_LOW + X1_SIZE)
#define Y1_SIZE  30
#define Y1_LOW   36
#define Y1_HIGH  (Y1_LOW + Y1_SIZE)

// 42 x 42
#define X2_SIZE  42
#define X2_LOW   114
#define X2_HIGH  (X2_LOW + X2_SIZE)
#define Y2_SIZE  42
#define Y2_LOW   168
#define Y2_HIGH  (Y2_LOW + Y2_SIZE)

#define X1_CUT  1
#define Y1_CUT  1
#define X2_CUT  1
#define Y2_CUT  1


class Entangled
{
public:
    Entangled(TString fileName, TString tree = "rawtree", UInt_t maxEntries = 0);
    ~Entangled();
    void Process();

private:
    void Init(TString file, TString tree);

    Bool_t PositionCheck(UInt_t area[4]);
    UInt_t FindPairs(UInt_t area[4], Int_t &entry);
    void ScanEntry(Int_t &entry);

private:
    TString inputName_, outputName_;
    Int_t nThreads_;
    UInt_t maxEntries_;
    TStopwatch time_;

    TFile* fileRoot_;
    TTree* rawTree_;

    TFile* outputRoot_;
    TDirectory* combinationDir_[X1_CUT][Y1_CUT][X2_CUT][Y2_CUT];
    TTree* entTree_[X1_CUT][Y1_CUT][X2_CUT][Y2_CUT];

    UInt_t      id_;
    UInt_t      mainEntry_;
    UInt_t      nextEntry_;

    UInt_t      Size_;
    UInt_t      Cols_[MAX_HITS];
    UInt_t      Rows_[MAX_HITS];
    ULong64_t   ToAs_[MAX_HITS];
    UInt_t      ToTs_[MAX_HITS];

    UInt_t      Size2_;
    UInt_t      Cols2_[MAX_HITS];
    UInt_t      Rows2_[MAX_HITS];
    ULong64_t   ToAs2_[MAX_HITS];
    UInt_t      ToTs2_[MAX_HITS];

    Int_t       Entries_;

    TString     inputFiles_[MAX_FILES];
    TString     inputDir_;
    Int_t       numberOfFiles_;

    UInt_t area1All_[4] = {X1_LOW, X1_HIGH, Y1_LOW, Y1_HIGH};
    UInt_t area2All_[4] = {X2_LOW, X2_HIGH, Y2_LOW, Y2_HIGH};
    UInt_t area1_[X1_CUT][Y1_CUT][4];// = {X1_LOW, X1_HIGH, Y1_LOW, Y1_HIGH};
    UInt_t area2_[X2_CUT][Y2_CUT][4];// = {X2_LOW, X2_HIGH, Y2_LOW, Y2_HIGH};

};

#endif // ENTANGLED_H
