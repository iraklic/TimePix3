#ifndef DATAPROCESS_H
#define DATAPROCESS_H

#include "TString.h"
#include "TStopwatch.h"

#include "TTree.h"
#include "TH1.h"
#include "TH2.h"

enum ProcType
{
    procDat     = 0,
    procRoot    = 1,
    procAll     = 2
};

class DataProcess
{
public:
    Int_t DataProcess(ProcType process, UInt_t nEntries);
    Int_t readOptions(Bool_t bCol, Bool_t bRow, Bool_t bToT, Bool_t bToA, Bool_t bTrig, Bool_t bTrigToA, Int_t nHitsCut, Bool_t noTrigWindow, Int_t windowCut, Bool_t singleFile, Int_t linesPerFile);
    Int_t processDat();
    Int_t processRoot();
    void plotStandardData();

private:
    void processFileNames();

    Int_t openDat();
    Int_t openRoot();

    void closeDat();
    void closeRoot();

private:
    //
    // global variables
    Bool_t m_bCol = kTRUE;
    Bool_t m_bRow = kTRUE;
    Bool_t m_bToT = kTRUE;
    Bool_t m_bToA = kTRUE;
    Bool_t m_bTrig = kTRUE;
    Bool_t m_bTrigToA = kTRUE;

    Bool_t m_noTrigWindow = kFALSE;
    Bool_t m_singleFile = kFALSE;

    Int_t m_nHitsCut = 0;
    Int_t m_windowCut = 40;
    Int_t m_linesPerFile = 100000;

    ProcType    m_process;
    UInt_t      m_maxEntries;
    TStopwatch  m_time;


    //
    // file names
    TString m_fileNameInput;
    TString m_fileNameDat;
    TString m_fileNameRoot;
    TString m_fileNamePdf;
    TString m_fileNameCsv;

    //
    // files
    FILE*   m_fileDat;
    TFile*  m_fileRoot;

    //
    // ROOT trees
    TTree* m_rawTree;
    TTree* m_timeTree;
    TTree* m_longTimeTree;

    const Int_t m_nRaw;
    const Int_t m_nTime;

    //
    // ROOT histograms
    TH2I* m_pixelMap;
    TH2F* m_pixelMapToT;
    TH2F* m_pixelMapToA;

    TH2I* m_ToAvsToT;

    TH1I* m_histToT;
    TH1I* m_histToA;
    TH1I* m_histTrigger;

    //
    // single pixel data
    UInt_t      m_row;
    UInt_t      m_col;
    UInt_t      m_ToT;
    ULong64_t   m_ToA;

    //
    // trigger data
    UInt_t      m_trigCnt;
    ULong64_t   m_trigTime;
};

#endif // DATAPROCESS_H
