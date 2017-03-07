#ifndef DATAPROCESS_H
#define DATAPROCESS_H

#include "TString.h"

#include "TTree.h"
#include "TH1.h"
#include "TH2.h"

class DataProcess
{
public:
    void DataProcess(UInt_t nEntries);
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
    // global variables - max number of entries, variable that decides if opening dat or root file
    //    Int_t var; //??
    UInt_t   m_maxEntries;

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
