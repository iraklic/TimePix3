#ifndef DATAPROCESS_H
#define DATAPROCESS_H

#include <deque>

#include "TSysEvtHandler.h"

#include "TObjString.h"
#include "TString.h"
#include "TStopwatch.h"

#include "TTree.h"
#include "TH1.h"
#include "TH2.h"

#define MAXHITS 100000       // minimal value for sorting 65k

using namespace std;

enum ProcType
{
    procDat     = 0,
    procRoot    = 1,
    procAll     = 2
};

class DataProcess
{
public:
    DataProcess(TString fileNameInput = "");
    void setName(TString fileNameInput);
    void setName(TObjString *fileNameInput, Int_t size);
    void setProcess(ProcType process);
    void setNEntries(UInt_t nEntries);
    Int_t setOptions(Bool_t bCol = kTRUE,
                      Bool_t bRow = kTRUE,
                      Bool_t bToT = kTRUE,
                      Bool_t bToA = kTRUE,
                      Bool_t bTrig = kTRUE,
                      Bool_t bTrigTime = kTRUE,
                      Bool_t bTrigToA = kTRUE,
                      Bool_t bNoTrigWindow = kFALSE,
                      ULong64_t timeWindow = 6000000,
                      Bool_t singleFile = kFALSE,
                      Int_t  linesPerFile = 100000);
    Int_t process();
    void StopLoop();

private:
    Int_t processFileNames();
    void finishMsg(TString fileName, TString operation, UInt_t events, Int_t fileCounter = 1);
    ULong64_t roundToNs(ULong64_t number);

    Int_t openDat(Int_t fileCounter = 0);
    Int_t openCsv(TString fileCounter = "");
    Int_t openRoot();

    Int_t processDat();
    Int_t skipHeader();

    Int_t processRoot();
    void plotStandardData();

    void closeDat();
    void closeCsv();
    void closeRoot();

private:
    //
    // global variables
    ProcType    m_process;
    UInt_t      m_maxEntries;
    TStopwatch  m_time;
    UInt_t      m_pixelCounter;
    UInt_t      m_lineCounter;
    TSignalHandler* m_sigHandler;
    Bool_t      m_bFirstTrig;
    Bool_t      m_bDevID;

    //
    // options of GUI
    Bool_t m_bCol;
    Bool_t m_bRow;
    Bool_t m_bToT;
    Bool_t m_bToA;
    Bool_t m_bTrig;
    Bool_t m_bTrigTime;
    Bool_t m_bTrigToA;
    Bool_t m_bNoTrigWindow;
    Bool_t m_bSingleFile;

    ULong64_t   m_timeWindow;
    UInt_t      m_linesPerFile;

    //
    // file names
    Int_t           m_numInputs;
    deque<TString > m_fileNameInput;
    TString         m_fileNamePath;
    deque<TString > m_fileNameDat;
    TString         m_fileNameRoot;
    TString         m_fileNamePdf;
    TString         m_fileNameCsv;

    //
    // files
    deque<FILE* >   m_filesDat;
    TFile*          m_fileRoot;
    deque<FILE* >   m_filesCsv;

    //
    // ROOT trees
    TTree* m_rawTree;
    TTree* m_timeTree;

    UInt_t m_nRaw;
    UInt_t m_nTime;

    //
    // ROOT histograms
    TH2I* m_pixelMap;
    TH2F* m_pixelMapToT;
    TH2F* m_pixelMapToA;

    TH1I* m_histToT;
    TH1I* m_histToA;
    TH2I* m_ToAvsToT;

    TH1I* m_histTrigger;
    TH1I* m_histTriggerToA;

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
